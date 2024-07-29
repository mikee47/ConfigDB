#!/usr/bin/env python3
#
# Sming hardware configuration tool
#

import argparse
import os
import sys
import json
from dataclasses import dataclass, field

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
    store: 'Store' = None
    properties: dict = field(default_factory=dict)
    children: list['Object'] = field(default_factory=list)

    @property
    def database(self):
        return self.parent.database

    @property
    def typename(self):
        return make_typename(self.name)

    @property
    def namespace(self):
        obj = self.parent
        ns = []
        while obj:
            ns.append(obj.typename)
            obj = obj.parent
        return '::'.join(ns)

    @property
    def id(self):
        return make_identifier(self.name)

    @property
    def base_class(self):
        return f'{self.store.store_ns}::Object'

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name

    @property
    def relative_path(self):
        path = self.path.removeprefix(self.store.path)
        return path.removeprefix('.')

@dataclass
class Store(Object):
    store_ns: str = None

    @property
    def typename(self):
        typename = super().typename if self.name else 'Root'
        return typename + 'Store'

    @property
    def base_class(self):
        return f'{self.store_ns}::Store'

@dataclass
class Database(Object):
    stores: list[Store] = field(default_factory=list)

    @property
    def database(self):
        return self

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
    return f'_F("{s}")' if s else 'nullptr'


def load_config(filename: str) -> Database:
    '''Load JSON configuration schema and parse into python objects
    '''
    with open(filename, 'r') as f:
        config = json.load(f)

    dbname = os.path.splitext(os.path.basename(filename))[0]
    db = Database(None, dbname, config['properties'])
    store_ns = config.get('store')
    if store_ns is None:
        raise RuntimeError(f'Database requires root "store" value')
    db.store = Store(db, '', store_ns=make_typename(store_ns))
    db.stores.append(db.store)
    config['object'] = db

    def parse(obj: Object, cfg: dict):
        for key, value in cfg['properties'].items():
            if value['type'] != 'object':
                continue
            properties = value['properties']
            if store_ns := value.get('store'):
                store = Store(obj, key, store_ns=make_typename(store_ns))
                db.stores.append(store)
            else:
                store = obj.store
            child = Object(obj, key, store, properties)
            obj.children.append(child)
            if store != obj.store or store == db.store:
                store.children.append(child)
            parse(child, value)

    parse(db, config)
    return db


def generate_database(db: Database) -> list:
    # Find out what stores are in use
    store_namespaces = set([child.store.store_ns for child in db.children])
    store_namespaces.add(db.store.store_ns)

    output = [
        '/****',
        ' *',
        ' * This file is auto-generated.',
        ' *',
        ' ****/',
        '',
        *[f'#include <ConfigDB/{typ}.h>' for typ in store_namespaces],
        '',
        f'class {db.typename}: public ConfigDB::Database',
        '{',
        'public:',
    ]
    for store in db.stores:
        output += [[
            '',
            f'class {store.typename}: public ConfigDB::StoreTemplate<ConfigDB::{store.store_ns}::Store, {store.typename}>',
            '{',
            'public:',
            [
                f'{store.typename}(ConfigDB::Database& db): StoreTemplate(db, {make_string(store.path)})',
                '{',
                '}',
                '',
                'std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override;',
            ],
            '};',
        ]]
    output += [generate_object(child) for child in db.children]
    output += [
        [
            '',
            f'{db.typename}(const String& path): Database(path)',
            '{',
            '}',
            '',
            'std::shared_ptr<ConfigDB::Store> getStore(unsigned index) override',
            '{',
            [
                'switch(index) {',
                [f'case {i}: return {store.typename}::open(*this);' for i, store in enumerate(db.stores)],
                ['default: return nullptr;'],
                '}',
            ],
            '}',
        ],
        '};'
    ]
    for store in db.stores:
        output += [
            '',
            f'std::unique_ptr<ConfigDB::Object> {store.namespace}::{store.typename}::getObject(unsigned index)',
            '{',
            [
                'switch(index) {',
                [f'case {i}: return std::make_unique<{obj.namespace}::{obj.typename}>(getPointer());' for i, obj in enumerate(store.children)],
                ['default: return nullptr;'],
                '}',
            ],
            '}',
        ]

    return output


def generate_object(obj: Object) -> list:
    output = [
        '',
        f'class {obj.typename}: public ConfigDB::{obj.base_class}',
        '{',
        'public:',
    ]

    # Append child object definitions
    output += [generate_object(child) for child in obj.children]

    init_list = [f'Object(store, {make_string(obj.relative_path)})']
    init_list += [f'{child.id}(store)' for child in obj.children]
    output += [[
        '',
        f'{obj.typename}(std::shared_ptr<{obj.store.typename}> store): {", ".join(init_list)}',
        '{',
        '}',
        '',
        f'{obj.typename}({obj.database.typename}& db): {obj.typename}({obj.store.typename}::open(db))',
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
        output += [[
            '',
            f'{ctype} get{make_typename(key)}()',
            '{',
            [f'return getValue<{ctype}>({make_string(key)});'],
            '}'
            '',
            f'bool set{make_typename(key)}(const {ctype}& value)',
            '{',
            [f'return setValue({make_string(key)}, value);'],
            '}'
        ]]

    output += [
        '',
        [f'{child.typename} {child.id};' for child in obj.children],
        '};'
    ]
    return output


def main():
    parser = argparse.ArgumentParser(description='ConfigDB specification utility')

    parser.add_argument('cfgfile', help='Path to configuration file')
    parser.add_argument('outdir', help='Output directory')

    args = parser.parse_args()

    db = load_config(args.cfgfile)

    output = generate_database(db)

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


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(repr(e), file=sys.stderr)
        raise
        sys.exit(2)
