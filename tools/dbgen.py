#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json
from dataclasses import dataclass

CPP_TYPENAMES = {
    'object': '-',
    'array': None,
    'string': 'String',
    'integer': 'int',
    'boolean': 'bool',
}

@dataclass
class Object:
    parent: 'Object'
    name: str
    properties: dict
    children: list

    @property
    def typename(self):
        return make_typename(self.name)

    @property
    def id(self):
        return make_identifier(self.name)

    @property
    def store(self):
        return self.parent.store

    @property
    def base_class(self):
        return f'{self.store.namespace}::Object'

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name

@dataclass
class Store(Object):
    namespace: str

    @property
    def store(self):
        return self

    @property
    def path(self):
        return ''

    @property
    def base_class(self):
        return f'{self.namespace}::Store'

@dataclass
class Database(Object):
    default_store: Store

    @property
    def store(self):
        return self.default_store

    @property
    def path(self):
        return ''

    @property
    def base_class(self):
        return 'Database'


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


def join_path(path: str, name: str):
    if path:
        path += '.'
    return path + name


def make_typename(s: str):
    '''Form valid CamelCase type name'''
    return make_identifier(s, True)


def make_string(s: str):
    '''Encode a string value'''
    return f'_F("{s}")'


def load_config(filename: str) -> Database:
    '''Load JSON configuration schema and parse into python objects
    '''
    with open(filename, 'r') as f:
        config = json.load(f)

    dbname = os.path.splitext(os.path.basename(filename))[0]
    db = Database(None, dbname, config['properties'], [], None)
    store_ns = config.get('store')
    if store_ns is None:
        raise RuntimeError(f'Database requires default "store" value')
    db.default_store = Store(db, 'defaultStore', [], None, make_typename(store_ns))
    config['object'] = db

    def parse(obj: Object, cfg: dict):
        for key, value in cfg['properties'].items():
            if value['type'] != 'object':
                continue
            properties = value['properties']
            if store_ns := value.get('store'):
                child_object = Store(obj, key, properties, [], make_typename(store_ns))
            else:
                child_object = Object(obj, key, properties, [])
            obj.children.append(child_object)
            parse(child_object, value)

    parse(db, config)
    return db


def parse_object(obj: Object) -> list:
    output = [
        '',
        f'class {obj.typename}: public ConfigDB::{obj.base_class}',
        '{',
        'public:',
    ]

    # Append child object definitions
    output += [parse_object(child) for child in obj.children]

    if isinstance(obj, Database):
        tmp = [f'Database(path)']
        for child in obj.children:
            if isinstance(child, Store):
                tmp += [f'{child.id}(*this)']
            else:
                tmp += [f'{child.id}(*this)']
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{obj.typename}(const String& path):',
            init_list,
            '{',
            '}',
        ]]
    elif isinstance(obj, Store):
        tmp = [f'Store(db, {make_string(obj.name)})']
        tmp += [f'{child.id}(*this)' for child in obj.children]
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{obj.typename}(ConfigDB::Database& db):',
            init_list,
            '{',
            '}',
        ]]
    else:
        tmp = [f'Object(store, {make_string(obj.path)})']
        tmp += [f'{child.id}(store)' for child in obj.children]
        init_list = [f'{x},' for x in tmp[:-1]] + [tmp[-1]]
        output += [[
            '',
            f'{obj.typename}(ConfigDB::{obj.store.base_class}& store):',
            init_list,
            '{',
            '}',
        ]]
    for key, value in obj.properties.items():
        ptype = value['type']
        ctype = CPP_TYPENAMES.get(ptype)
        if not ctype:
            print(f'*** "{obj.path}": {ptype} type not yet implemented.')
            continue
        if ctype == '-':
            continue
        output += [parse_basic(obj.path, key, ctype)]
    output += [
        '',
        [f'{child.typename} {child.id};' for child in obj.children],
        '};'
    ]
    return output


def parse_array(config: dict) -> list:
    # TODO
    return []


def parse_basic(path: str, value_name: str, value_type: str) -> list:
    value_path = join_path(path, value_name)
    return [
        '',
        f'{value_type} get{make_typename(value_name)}()',
        '{',
        [
            f'return getValue<{value_type}>({make_string(value_path)});',
        ],
        '}'
        '',
        f'bool set{make_typename(value_name)}(const {value_type}& value)',
        '{',
        [
            f'return setValue({make_string(value_path)}, value);'
        ],
        '}'
    ]


def main():
    parser = argparse.ArgumentParser(description='ConfigDB specification utility')

    parser.add_argument('cfgfile', help='Path to configuration file')
    parser.add_argument('outdir', help='Output directory')

    args = parser.parse_args()

    db = load_config(args.cfgfile)

    # Find out what stores are in use
    store_namespaces = set([child.store.namespace for child in db.children])
    store_namespaces.add(db.store.namespace)
    print(f'stores: {store_namespaces}')

    output = [
        '/****',
        ' *',
        ' * This file is auto-generated.',
        ' *',
        ' ****/',
        '',
        *[f'#include <ConfigDB/{typ}.h>' for typ in store_namespaces],
        *parse_object(db)
    ]

    filename = os.path.join(args.outdir, f'{db.name}.h')
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

def test_load() -> dict:
    with open('../samples/Basic_Config/basic-config.cfgdb') as f:
        config = json.load(f)
    parse_config('basic-config', config)
    return config

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(repr(e), file=sys.stderr)
        raise
        sys.exit(2)
