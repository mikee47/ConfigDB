#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json

STORE_KIND = 'Json'

class Context:
    def __init__(self, path_list: list, name: str):
        self.name = name
        self.path_list = path_list

    @property
    def path(self):
        return '.'.join(self.path_list)

    @property
    def type_name(self):
        return make_type_name(self.name)


def make_identifier(s: str, is_type: bool = False):
    '''Make camelcase identifier for a type definition or variable'''
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
    return make_identifier(s, True)


def make_string(s: str):
    return f'"{s}"'
    # return f'_F("{s}")'


def parse_object(ctx: Context, config: dict, output: list):
    # Get list of any immediate child objects which we should instantiate
    children = []
    for key, value in config['properties'].items():
        if value['type'] == 'object':
            children.append(key)
    if children:
        print('Children: ' + ', '.join(children))

    tmp = [f'Group(store, {make_string(ctx.path)})']
    tmp += [f'{make_identifier(c)}(store)' for c in children]
    init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]

    output += [
        '',
        f'class {ctx.type_name}: public ConfigDB::{STORE_KIND}::Group',
        '{',
        'public:',
        [
            f'{ctx.type_name}(ConfigDB::{STORE_KIND}::Store& store):',
            init_list,
            '{',
            '}',
            '',
        ]
    ]
    for key, value in config['properties'].items():
        ptype = value['type']
        parse = type_parsers[ptype]
        out = []
        parse(Context(ctx.path_list + [key], key), value, out)
        output += [ out ]
    output += [
        '',
        [f'{make_type_name(c)} {make_identifier(c)};' for c in children],
        '};'
    ]

def parse_array(ctx: Context, config: dict, output: list):
    pass

def parse_basic(ctx: Context, config: dict, output: list, value_type: str):
    output += [
        '',
        f'{value_type} get{ctx.type_name}()',
        '{',
        [
            f'return Group::getValue<{value_type}>({make_string(ctx.name)});',
        ],
        '}'
        '',
        f'bool set{ctx.type_name}(const {value_type}& value)',
        '{',
        [
            f'return Group::setValue({make_string(ctx.name)}, value);'
        ],
        '}'
    ]

def parse_string(ctx: Context, config: dict, output: list):
    parse_basic(ctx, config, output, 'String')

def parse_integer(ctx: Context, config: dict, output: list):
    parse_basic(ctx, config, output, 'int')

def parse_boolean(ctx: Context, config: dict, output: list):
    parse_basic(ctx, config, output, 'bool')

type_parsers = {
    'object': parse_object,
    'array': parse_array,
    'string': parse_string,
    'integer': parse_integer,
    'boolean': parse_boolean,
}


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
    ]
    parse_object(Context([], name), config, output)

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
