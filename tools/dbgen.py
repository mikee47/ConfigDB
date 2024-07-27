#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json

class Context:
    def __init__(self, path: list, name: str):
        self.path_list = list(path) + [name]

    @property
    def name(self):
        _, _, s = self.path.rpartition('.')
        return s

    @property
    def path(self):
        return '.'.join(self.path_list)

    @property
    def type_name(self):
        return make_type_name(self.name)


def make_type_name(s: str):
    up = True
    name = ''
    for c in s:
        if c in ['-', '_']:
            up = True
        else:
            name += c.upper() if up else c
            up = False
    return name


def make_string(s: str):
    return f'"{s}"'
    # return f'_F("{s}")'


def parse_object(ctx: Context, config: dict, output: list):
    output += [
        '',
        f'class {ctx.type_name}: public ConfigDB::Group',
        '{',
        'public:',
        [
            f'{ctx.type_name}(ConfigDB::Config& db): Group(db, {make_string(ctx.path)})',
            '{',
            '}',
        ],
    ]
    for key, value in config['properties'].items():
        ptype = value['type']
        parse = type_parsers[ptype]
        out = []
        parse(Context(ctx.path_list, key), value, out)
        output += [ out ]
    output += [
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
            f'return Group::getValue({make_string(ctx.name)});'
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

    parser.add_argument('input', help='Path to configuration file')
    parser.add_argument('output', help='Output directory')

    args = parser.parse_args()

    with open(args.input, 'r') as f:
        config = json.load(f)

    name = os.path.splitext(os.path.basename(args.input))[0]
    output = []
    parse_object(Context([], name), config, output)

    def dump_output(items: list, indent: str):
        for item in items:
            if isinstance(item, list):
                dump_output(item, indent + '    ')
            else:
                print(f'{indent}{item}')
    dump_output(output, '')


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
