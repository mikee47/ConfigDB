#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json

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
        return make_typename(self.name)


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


def make_typename(s: str):
    '''Form valid CamelCase type name'''
    return make_identifier(s, True)


def make_string(s: str):
    '''Encode a string value'''
    return f'_F("{s}")'


def parse_object(location: JsonPath, store_kind: str, config: dict) -> list:
    is_database = ('database' in config)
    is_store = ('store' in config)
    if is_database:
        if not is_store:
            raise RuntimeError(f'Database requires default "store" value')
        store_kind = make_typename(config['store'])
        baseClass = 'Database'
    elif is_store:
        store_kind = make_typename(config['store'])
        baseClass = f'{store_kind}::Store'
    else:
        baseClass = f'{store_kind}::Group'

    if is_database:
        location.nodes = []

    output = [
        '',
        f'class {location.type_name}: public ConfigDB::{baseClass}',
        '{',
        'public:',
    ]

    # Get list of any immediate child objects which we should instantiate
    children = []
    for key, value in config['properties'].items():
        if value['type'] == 'object':
            loc = JsonPath(location.nodes + [key], key)
            output += [parse_object(loc, store_kind, value)]
            children.append(key)

    if is_database:
        tmp = [f'Database(path)']
        tmp += [f'{make_identifier(c)}(store)' for c in children]
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{location.type_name}(const String& path):',
            init_list,
            '{',
            '}',
        ]]
    elif is_store:
        tmp = [f'Store(db, {make_string(key)})']
        tmp += [f'{make_identifier(c)}(store)' for c in children]
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{location.type_name}(ConfigDB::Database& db):',
            init_list,
            '{',
            '}',
        ]]
    else:
        tmp = [f'Group(store, {make_string(location.path)})']
        tmp += [f'{make_identifier(c)}(store)' for c in children]
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{location.type_name}(ConfigDB::{store_kind}::Store& store):',
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
        [f'{make_typename(c)} {make_identifier(c)};' for c in children],
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
            f'return getValue<{value_type}>({make_string(location.name)});',
        ],
        '}'
        '',
        f'bool set{location.type_name}(const {value_type}& value)',
        '{',
        [
            f'return setValue({make_string(location.name)}, value);'
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
    # Top-level object is implicitly a database
    config['database'] = True

    # Find out what stores are in use
    stores = set()
    def find_stores(cfg: dict):
        kind = cfg.get('store')
        if kind:
            stores.add(kind)
        for x in cfg.values():
            if isinstance(x, dict):
                find_stores(x)
    find_stores(config)    
    print(f'stores: {stores}')

    name = os.path.splitext(os.path.basename(args.cfgfile))[0]
    output = [
        '/****',
        ' *',
        ' * This file is auto-generated.',
        ' *',
        ' ****/',
        '',
        *[f'#include <ConfigDB/{make_typename(kind)}.h>' for kind in stores],
        *parse_object(JsonPath([name], name), None, config)
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
