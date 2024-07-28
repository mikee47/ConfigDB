#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json

STORE_KIND = 'Json'

CPP_TYPENAMES = {
    'object': '-',
    'array': None,
    'string': 'String',
    'integer': 'int',
    'boolean': 'bool',
}

class JsonPath:
    '''Manages node identifier path'''
    def __init__(self, nodes: list, name: str):
        self.name = name
        self.nodes = nodes

    @property
    def path(self):
        return '.'.join(self.nodes)

    @property
    def type_name(self):
        return make_type_name(self.name)


def make_identifier(s: str, is_type: bool = False):
    '''Form valid camelCase identifier for a variable (default) or type'''
    up = is_type
    id = ''
    for c in s:
        if c in ['-', '_']:
            up = True
        else:
            id += c.upper() if up else c
            up = False
    return id


def make_type_name(s: str):
    '''Form valid CamelCase type name'''
    return make_identifier(s, True)


def make_string(s: str):
    '''Encode a string value'''
    return f'_F("{s}")'


def parse_object(location: JsonPath, config: dict) -> list:
    output = [
        '',
        f'class {location.type_name}: public ConfigDB::{STORE_KIND}::Group',
        '{',
        'public:',
    ]

    # Get list of any immediate child objects which we should instantiate
    children = []
    for key, value in config['properties'].items():
        if value['type'] == 'object':
            loc = JsonPath(location.nodes + [key], key)
            output += [parse_object(loc, value)]
            children.append(key)

    tmp = [f'Group(store, {make_string(location.path)})']
    tmp += [f'{make_identifier(c)}(store)' for c in children]
    init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]

    output += [[
        '',
        f'{location.type_name}(ConfigDB::{STORE_KIND}::Store& store):',
        init_list,
        '{',
        '}',
    ]]
    for key, value in config['properties'].items():
        loc = JsonPath(location.nodes + [key], key)
        ptype = value['type']
        ctype = CPP_TYPENAMES.get(ptype)
        if not ctype:
            print(f'*** "{loc.path}": {ptype} type not yet implemented.')
            continue
        if ctype == '-':
            continue
        output += [parse_basic(loc, value, ctype)]
    output += [
        '',
        [f'{make_type_name(c)} {make_identifier(c)};' for c in children],
        '};'
    ]
    return output


def parse_array(location: JsonPath, config: dict) -> list:
    # TODO
    return []


def parse_basic(location: JsonPath, config: dict, value_type: str) -> list:
    return [
        '',
        f'{value_type} get{location.type_name}()',
        '{',
        [
            f'return Group::getValue<{value_type}>({make_string(location.name)});',
        ],
        '}'
        '',
        f'bool set{location.type_name}(const {value_type}& value)',
        '{',
        [
            f'return Group::setValue({make_string(location.name)}, value);'
        ],
        '}'
    ]


def main():
    parser = argparse.ArgumentParser(description='ConfigDB specification utility')

    parser.add_argument('cfgfile', help='Path to configuration file')
    parser.add_argument('outdir', help='Output directory')

    args = parser.parse_args()

    with open(args.cfgfile, 'r') as f:
        config = json.load(f)

    name = os.path.splitext(os.path.basename(args.cfgfile))[0]
    output = [
        '/****',
        ' *',
        ' * This file is auto-generated.',
        ' *',
        ' ****/',
        '',
        f'#include <ConfigDB/{STORE_KIND}.h>',
        parse_object(JsonPath([], name), config)
    ]

    filename = os.path.join(args.outdir, f'{name}.h')
    os.makedirs(args.outdir, exist_ok=True)
    with open(filename, 'w') as f:
        def dump_output(items: list, indent: str):
            for item in items:
                if isinstance(item, list):
                    dump_output(item, indent + '    ')
                elif item:
                    f.write(f'{indent}{item}\n')
                else:
                    f.write('\n')
        dump_output(output, '')


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(repr(e), file=sys.stderr)
        raise
        sys.exit(2)
