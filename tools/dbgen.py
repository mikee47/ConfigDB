'''ConfigDB database generator'''

import argparse
import os
import sys
import json
import re
from dataclasses import dataclass, field

CPP_TYPENAMES = {
    'object': '-',
    'array': None,
    'string': 'String',
    'integer': 'int',
    'boolean': 'bool',
}

strings: dict[str, str] = {}

def get_string(value: str, null_if_empty: bool = False) -> str:
    '''All string data stored as FSTR values
    This de-duplicates strings and returns a unique C++ identifier to be used.
    '''
    if value is None or (null_if_empty and value == ''):
        return 'nullptr'
    ident = strings.get(value)
    if ident:
        return ident
    ident = re.sub(r'[^A-Za-z0-9-]+', '', value)[:10]
    if not ident:
        ident = str(len(strings))
    ident = 'fstr_' + ident
    if ident in strings.values():
        i = 0
        while f'{ident}_{i}' in strings.values():
            i += 1
        ident = f'{ident}_{i}'
    strings[value] = ident
    return ident



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
            ns.insert(0, obj.typename)
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

    @property
    def contained_children(self):
        return (child for child in self.children if child.store == self.store)


@dataclass
class Store(Object):
    store_ns: str = None

    @property
    def typename(self):
        typename = super().typename if self.name else 'Root'
        return typename + 'Store'

    @property
    def namespace(self):
        obj = self
        while obj.parent:
            obj = obj.parent
        return obj.typename

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


@dataclass
class CodeLines:
    '''Code is generated as list, with nested lists indented 
    '''
    header: list[str | list] = field(default_factory=list)
    source: list[str | list] = field(default_factory=list)

    def append(self, other: 'CodeLines'):
        self.header += [other.header]
        self.source += other.source


def make_identifier(s: str, is_type: bool = False):
    '''Form valid camelCase identifier for a variable (default) or type'''
    up = is_type
    ident = ''
    for c in s:
        if c in ['-', '_']:
            up = True
        else:
            ident += c.upper() if up else c
            up = False
    return ident


def join_path(path: str, name: str):
    '''Make path string using JSONPath syntax'''
    if path:
        path += '.'
    return path + name


def make_typename(s: str):
    '''Form valid CamelCase type name'''
    return make_identifier(s, True)


def make_string(s: str):
    '''Encode a string value'''
    return f'"{s.replace('"', '\\"')}"'


def load_schema(filename: str) -> dict:
    '''Load JSON configuration schema and validate
    '''
    with open(filename, 'r', encoding='utf-8') as f:
        config = json.load(f)
    try:
        from jsonschema import Draft7Validator
        v = Draft7Validator(Draft7Validator.META_SCHEMA)
        errors = list(v.iter_errors(config))
        if errors:
            for e in errors:
                print(f'{e.message} @ {e.path}')
            sys.exit(3)
    except ImportError as err:
        print(f'\n** WARNING! {err}: Cannot validate "{filename}", please run `make python-requirements` **\n\n')
    return config


def load_config(filename: str) -> Database:
    '''Load, validate and parse schema into python objects'''
    config = load_schema(filename)
    dbname = os.path.splitext(os.path.basename(filename))[0]
    db = Database(None, dbname, config['properties'])
    store_ns = config.get('store')
    if store_ns is None:
        raise RuntimeError('Database requires root "store" value')
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


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''
    # Find out what stores are in use
    store_namespaces = {child.store.store_ns for child in db.children}
    store_namespaces.add(db.store.store_ns)

    lines = CodeLines()
    lines.source = [
        f'#include "{db.name}.h"',
        '',
        'using namespace ConfigDB;',
    ]
    for store in db.stores:
        get_child_objects = generate_method_get_child_object(store, 'getPointer()')
        lines.header += [[
            '',
            '/**',
            ' *',
            f' * @class: {store.typename}',
            f' * @brief: ConfigDB store class for {store.typename} ',
            ' *',
            ' */',
            f'class {store.typename}: public ConfigDB::StoreTemplate<ConfigDB::{store.base_class}, {store.typename}>',
            '{',
            'public:',
            [
                f'{store.typename}(ConfigDB::Database& db): StoreTemplate(db, {get_string(store.path, True)})',
                '{',
                '}',
                '',
                *get_child_objects.header,
            ],
            '};',
        ]]
        lines.source += get_child_objects.source
    for child in db.children:
        lines.append(generate_object(child))
    lines.header += [
        [
            '',
            f'{db.typename}(const String& path): Database(path)',
            '{',
            '}',
            '',
            'std::shared_ptr<ConfigDB::Store> getStore(unsigned index) override;',
        ],
        '};'
    ]

    lines.source += [
        '',
        f'std::shared_ptr<Store> {db.typename}::getStore(unsigned index)',
        '{',
        [
            'switch(index) {',
            [f'case {i}: return {store.typename}::open(*this);' for i, store in enumerate(db.stores)],
            ['default: return nullptr;'],
            '}',
        ],
        '}',
    ]

    lines.header = [
        *[f'#include <ConfigDB/{ns}/Object.h>' for ns in store_namespaces],
        '',
        '/**',
        ' *',
        f' * @class ConfigDB base class for {db.typename}, generated from schema',
        ' *',
        ' */',
        f'class {db.typename}: public ConfigDB::Database',
        '{',
        'public:',
        [f'DEFINE_FSTR_LOCAL({id}, {make_string(value)})' for value, id in strings.items()]
    ] + lines.header

    return lines


def generate_method_get_child_object(obj: Store | Object, store: str) -> CodeLines:
    '''Generate *getChildObject* method'''

    return CodeLines(
        [
            '',
            'std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override;'
        ],
        [
            '',
            f'std::unique_ptr<Object> {obj.namespace}::{obj.typename}::getObject(unsigned index)',
            '{',
            [
                'switch(index) {',
                [f'case {i}: return std::make_unique<{child.namespace}::{child.typename}>({store});'
                    for i, child in enumerate(obj.children)],
                ['default: return nullptr;'],
                '}',
            ],
            '}',
        ]
    )


def generate_method_get_property(obj: Object) -> CodeLines:
    '''Generate *getProperty* method'''

    lines = CodeLines(
        [
            '',
            'ConfigDB::Property getProperty(unsigned index) override;',
        ],
        [
            '',
            f'Property {obj.namespace}::{obj.typename}::getProperty(unsigned index)',
            '{',
            ['switch(index) {'],
        ])
    for i, (key, value) in enumerate(obj.properties.items()):
        ptype = value['type']
        ctype = CPP_TYPENAMES.get(ptype)
        if not ctype:
            print(f'*** "{obj.path}": {ptype} type not yet implemented.')
            continue
        if ctype == '-':
            continue
        default = value.get('default')

        # Source
        if default is None:
            default_str = 'nullptr'
        elif ptype == 'string':
            default_str = '&' + get_string(default)
        elif ptype == 'boolean':
            default_str = '&' + get_string('True' if default else 'False')
        else:
            default_str = '&' + get_string(str(default))
        lines.source += [[[
            f'case {i}: return {{*this, {get_string(key)}, Property::Type::{ptype.capitalize()}, {default_str}}};'
        ]]]

    lines.source += [
        [
            ['default: return {*this};'],
            '}',
        ],
        '}',
    ]

    return lines


def generate_method_accessors(obj: Object) -> CodeLines:
    '''Generate typed get/set methods for each property'''

    lines = CodeLines()
    for key, value in obj.properties.items():
        ptype = value['type']
        ctype = CPP_TYPENAMES.get(ptype)
        if not ctype:
            print(f'*** "{obj.path}": {ptype} type not yet implemented.')
            continue
        if ctype == '-':
            continue
        default = value.get('default')
        if default is None:
            default_str = ''
        elif ptype == 'string':
            default_str = f', {get_string(default)}'
        elif ptype == 'boolean':
            default_str = f', {'true' if default else 'false'}'
        else:
            default_str = f', {default}'

        if default_str != '':
            default_str_doxy=', default {default_str}'
        else:
            default_str_doxy=''

        lines.header += [
            '',
            f'/**',
            f' * @brief get{make_typename(key)}',
            f' * ',
            f' * get the value for {obj.path}.{key}',
            f' * ',
            rf' * @return `{ctype}`; the value for {obj.path}.{key}{default_str_doxy}',
            f' */',
            f'{ctype} get{make_typename(key)}() const',
            '{',
            [f'return getValue<{ctype}>({get_string(key)}{default_str});'],
            '}',
            '',
            f'/**',
            f' * @brief set{make_typename(key)}',
            f' * ',
            f' * set the value for {obj.path}.{key}',
            f' * ',
            rf' * @param value the `{ctype}` value for {obj.path}.{key}{default_str}',
            f' * @return bool success',
            f' */',
            f'bool set{make_typename(key)}(const {ctype}& value)',
            '{',
            [f'return setValue({get_string(key)}, value);'],
            '}'
        ]
    return lines


def generate_object(obj: Object) -> CodeLines:
    '''Generate code for Object implementation'''

    lines = CodeLines()
    lines.header = [
        '',
        '/**',
        ' *',
        f' *  @class ConfigDB class for {obj.typename}, generated from schema',
        ' *',
        ' */',
        f'class {obj.typename}: public ConfigDB::{obj.base_class}',
        '{',
        'public:',
    ]

    # Append child object definitions
    for child in obj.children:
        lines.append(generate_object(child))

    init_list = ', '.join([
        f'Object(store, {get_string(obj.relative_path, True)})',
        *(f'{child.id}(store)' for child in obj.contained_children)
    ])
    lines.header += [[
        '',
        f'{obj.typename}(std::shared_ptr<ConfigDB::{obj.store.base_class}> store): {init_list}',
        '{',
        '}',
        '',
        f'{obj.typename}({obj.database.typename}& db): {obj.typename}({obj.store.typename}::open(db))',
        '{',
        '}',
    ]]

    lines.append(generate_method_get_child_object(obj, 'store'))
    lines.append(generate_method_get_property(obj))
    lines.append(generate_method_accessors(obj))

    # Contained children member variables
    lines.header += [
        '',
        [f'{child.typename} {child.id};' for child in obj.contained_children],
        '};'
    ]

    return lines


def write_file(content: list[str | list], filename: str):
    comment = [
        '/****',
        ' *',
        ' * This file is auto-generated.',
        ' *',
        ' ****/',
        '',
    ]

    def dump_output(items: list, indent: str):
        for item in items:
            if isinstance(item, list):
                dump_output(item, indent + '    ')
            elif item:
                f.write(f'{indent}{item}\n')
            else:
                f.write('\n')

    with open(filename, 'w', encoding='utf-8') as f:
        dump_output(comment + content, '')


def main():
    parser = argparse.ArgumentParser(description='ConfigDB specification utility')

    parser.add_argument('cfgfile', help='Path to configuration file')
    parser.add_argument('outdir', help='Output directory')

    args = parser.parse_args()

    db = load_config(args.cfgfile)

    lines = generate_database(db)

    filepath = os.path.join(args.outdir, f'{db.name}')
    os.makedirs(args.outdir, exist_ok=True)

    write_file(lines.header, f'{filepath}.h')
    write_file(lines.source, f'{filepath}.cpp')


if __name__ == '__main__':
    main()
