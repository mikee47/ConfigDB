'''ConfigDB database generator'''

import argparse
import os
import sys
import json
import re
from dataclasses import dataclass, field

MAX_STRINGID_LEN = 32

CPP_TYPENAMES = {
    'object': '-',
    'array': '-',
    'string': 'String',
    'integer': 'int',
    'boolean': 'bool',
}

strings: dict[str, str] = {
    "": 'empty',
}

STRING_PREFIX = 'fstr_'

def get_string(value: str, null_if_empty: bool = False) -> str:
    '''All string data stored as FSTR values
    This de-duplicates strings and returns a unique C++ identifier to be used.
    '''
    if value is None or (null_if_empty and value == ''):
        return 'nullptr'
    ident = strings.get(value)
    if ident:
        return STRING_PREFIX + ident
    ident = re.sub(r'[^A-Za-z0-9-]+', '_', value)[:MAX_STRINGID_LEN]
    if not ident:
        ident = str(len(strings))
    if ident in strings.values():
        i = 0
        while f'{ident}_{i}' in strings.values():
            i += 1
        ident = f'{ident}_{i}'
    strings[value] = ident
    return STRING_PREFIX + ident


@dataclass
class Property:
    name: str
    fields: dict

    @property
    def ptype(self):
        return self.fields['type']

    @property
    def ctype(self):
        return CPP_TYPENAMES[self.ptype]

    @property
    def typename(self):
        return make_typename(self.name)

    @property
    def default(self):
        return self.fields.get('default')

    @property
    def default_str(self):
        default = self.default
        if self.ptype == 'string':
            return get_string(default)
        if self.ptype == 'boolean':
            return 'true' if default else 'false'
        return str(default) if default else '{}'

    @property
    def default_fstr(self):
        default = self.default
        if default is None:
            return 'nullptr'
        if self.ptype == 'string':
            return '&' + get_string(default)
        if self.ptype == 'boolean':
            return '&' + get_string('True' if default else 'False')
        return '&' + get_string(str(default))


@dataclass
class Object:
    parent: 'Object'
    name: str
    store: 'Store' = None
    properties: list[Property] = field(default_factory=list)
    children: list['Object'] = field(default_factory=list)

    @property
    def database(self):
        return self.parent.database

    @property
    def typename(self):
        return make_typename(self.name) if self.name else 'Root'

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
        return make_identifier(self.name) if self.name else 'root';

    @property
    def classname(self):
        return type(self).__name__

    @property
    def base_class(self):
        return f'{self.store.store_ns}::{self.classname}'

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name

    @property
    def relative_path(self):
        path = self.path.removeprefix(self.store.path)
        return path.removeprefix('.')

    @property
    def contained_children(self):
        return [child for child in self.children if child.store == self.store]


@dataclass
class Array(Object):
    items: Property = None

@dataclass
class ObjectArray(Object):
    items: Object = None

@dataclass
class Store(Object):
    store_ns: str = None

    @property
    def typename(self):
        return f'{super().typename}Store'

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
    header: list[str | list]
    source: list[str | list]

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


def declare_templated_class(obj: Object, tparams: list = []) -> list[str]:
    return [
        '',
        f'class {obj.typename}: public ConfigDB::{obj.base_class}Template<{", ".join(tparams + [obj.typename])}>',
        '{',
        'public:',
    ]


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
    db = Database(None, dbname, properties=config['properties'])
    store_ns = config.get('store')
    if store_ns is None:
        raise RuntimeError('Database requires root "store" value')
    db.store = Store(db, '', store_ns=make_typename(store_ns))
    db.stores.append(db.store)
    root = Object(db, '', db.store)
    db.children.append(root)

    def parse(obj: Object, properties: dict):
        obj_root = obj
        for key, value in properties.items():
            obj = obj_root
            # Filter out support property types
            prop = Property(key, value)
            if not prop.ctype:
                print(f'*** "{obj.path}": {prop.ptype} type not yet implemented.')
                continue
            if prop.ctype != '-': # object or array
                obj.properties.append(prop)
                continue
            if store_ns := value.get('store'):
                if obj is not root:
                    raise ValueError(f'{key} cannot have "store", not a root object')
                store = Store(db, key, store_ns=make_typename(store_ns))
                db.stores.append(store)
                obj = root
            else:
                store = obj.store
            if prop.ptype == 'object':
                child = Object(obj, key, store)
                parse(child, value['properties'])
            elif prop.ptype == 'array':
                items = value['items']
                if items['type'] == 'object':
                    arr = ObjectArray(obj, key, store)
                    arr.items = Object(arr.parent, f'{arr.typename}Item', store)
                    parse(arr.items, items['properties'])
                else:
                    arr = Array(obj, key, store)
                    parse(arr, {'items': items})
                    assert len(arr.properties) == 1
                    arr.items = arr.properties[0]
                    arr.properties = None
                child = arr
            else:
                raise ValueError('Bad type ' + repr(prop))
            obj.children.append(child)

    parse(root, db.properties)
    return db


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''
    # Find out what stores are in use
    store_namespaces = {child.store.store_ns for child in db.children}
    store_namespaces.add(db.store.store_ns)

    lines = CodeLines(
        [],
        [
            f'#include "{db.name}.h"',
            '',
            'using namespace ConfigDB;',
        ])
    for store in db.stores:
        lines.header += [[
            *declare_templated_class(store),
            [
                f'{store.typename}(ConfigDB::Database& db): StoreTemplate(db, {get_string(store.path, True)})',
                '{',
                '}',
            ],
            '};',
        ]]

    for child in db.children:
        lines.append(generate_object(child))

    lines.header += [
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

    lines.header[:0] = [
        *[f'#include <ConfigDB/{ns}/Store.h>' for ns in store_namespaces],
        '',
        f'class {db.typename}: public ConfigDB::Database',
        '{',
        'public:',
        [f'DEFINE_FSTR_LOCAL({STRING_PREFIX}{id}, {make_string(value)})' for value, id in strings.items()]
    ]

    return lines


def generate_method_get_child_object(obj: Store | Object) -> CodeLines:
    '''Generate *getChildObject* method'''

    return CodeLines(
        [
            '',
        	'unsigned getObjectCount() const override',
            '{',
            [f'return {len(obj.children)};'],
            '}',
            '',
            'std::unique_ptr<ConfigDB::Object> getObject(const String& key) override',
            '{',
            ['return nullptr;'],
            '}',
            '',
            'std::unique_ptr<ConfigDB::Object> getObject(unsigned index) override',
            '{',
            [
                'switch(index) {',
                [f'case {i}: return std::make_unique<Contained{child.typename}>(*this);'
                    for i, child in enumerate(obj.children)],
                ['default: return nullptr;'],
                '}',
            ],
            '}'
        ],
        []
    )


def generate_method_get_property(obj: Object) -> CodeLines:
    '''Generate *getProperty* method'''

    objbase = len(obj.properties)
    return CodeLines(
        [
            '',
        	'unsigned getPropertyCount() const override',
            '{',
            [f'return {len(obj.children) + len(obj.properties)};'],
            '}',
            '',
            'ConfigDB::Property getProperty(unsigned index) override',
            '{',
            [
                'using Type = ConfigDB::Property::Type;',
                'switch(index) {',
            ],
            [
                *(f'case {i}: return {{*this, {get_string(prop.name)}, Type::{prop.ptype.capitalize()}, {prop.default_fstr}}};'
                for i, prop in enumerate(obj.properties))
            ],
            [
                *(f'case {objbase+i}: return {{*this, {get_string(child.name)}, Type::{obj.classname}, nullptr}};'
                for i, child in enumerate(obj.children))
            ],
            [
                'default: return {};',
                '}',
            ],
            '}',
        ],
        [])


def generate_array_get_property(array: Array) -> CodeLines:
    '''Generate *getProperty* method for Array'''

    prop = array.items
    return CodeLines([
        '',
        'ConfigDB::Property getProperty(unsigned index) override',
        '{',
        [f'return getArrayProperty(index, ConfigDB::Property::Type::{prop.ptype.capitalize()}, {prop.default_fstr});'],
        '}',
    ], [])


def generate_object_accessors(obj: Object) -> list:
    '''Generate typed get/set methods for each property'''

    return [
        *((
            '',
            f'{prop.ctype} get{prop.typename}() const',
            '{',
            [f'return getValue<{prop.ctype}>({get_string(prop.name)}, {prop.default_str});'],
            '}',
            '',
            f'bool set{prop.typename}(const {prop.ctype}& value)',
            '{',
            [f'return setValue({get_string(prop.name)}, value);'],
            '}'
            ) for prop in obj.properties)
        ]


def generate_array_accessors(arr: Array) -> CodeLines:
    '''Generate typed get/set methods for each property'''

    prop = arr.items
    return CodeLines([
        '',
        f'{prop.ctype} getItem(unsigned index) const',
        '{',
        [f'return Array::getItem<{prop.ctype}>(index, {prop.default_str});'],
        '}',
        '',
        f'bool setItem(unsigned index, const {prop.ctype}& value)',
        '{',
        [f'return Array::setItem(index, value);'],
        '}'
    ], [])


def generate_object(obj: Object) -> CodeLines:
    '''Generate code for Object implementation'''

    if isinstance(obj, ObjectArray):
        item_lines = generate_item_object(obj.items)
        return CodeLines([
            '',
            f'class {obj.typename};',
            *item_lines.header,
            '',
            f'class Contained{obj.typename}: public ConfigDB::{obj.base_class}Template<{obj.typename}, {obj.items.typename}>',
            '{',
            'public:',
            [f'using {obj.classname}Template::{obj.classname}Template;'],
            '};',
            '',
            f'class {obj.typename}: public Contained{obj.typename}',
            '{',
            'public:',
            [
                f'{obj.typename}({obj.database.typename}& db):',
                [', '.join([
                    f'Contained{obj.typename}(*{obj.store.typename}::open(db), {get_string(obj.path)})',
                    f'store({obj.store.typename}::open(db))',
                    *(f'{child.id}(*this)' for child in obj.contained_children),
                ])],
                '{',
                '}',
                '',
                'ConfigDB::Store& getStore() override',
                '{',
                ['return *store;'],
                '}',
            ],
            '',
            [f'std::shared_ptr<{obj.store.typename}> store;'],
            '};',
        ],
        item_lines.source)


    lines = CodeLines(
        [
            '',
            f'class Contained{obj.typename}: public ConfigDB::{obj.base_class}',
            '{',
            'public:',
        ],
        [])

    # Append child object definitions
    for child in obj.children:
        lines.append(generate_object(child))

    lines.header += [[
        '',
        f'using {obj.classname}::{obj.classname};',
        '',
        f'Contained{obj.typename}({obj.store.typename}& store, const String& path):',
        [', '.join([
            f'{obj.classname}(store, path)',
            *(f'{child.id}(*this)' for child in obj.contained_children)
        ])],
        '{',
        '}'
    ]]

    if not isinstance(obj.parent, Database):
        lines.header += [[
            '',
            f'Contained{obj.typename}({obj.parent.classname}& parent):',
            [', '.join([
                f'{obj.classname}(parent)',
                *(f'{child.id}(*this)' for child in obj.contained_children)
            ])],
            '{',
            '}'
        ]]

    if isinstance(obj, Array):
        lines.append(generate_array_accessors(obj))
        lines.append(generate_array_get_property(obj))
    elif isinstance(obj, ObjectArray):
        assert False
    else:
        lines.header += generate_object_accessors(obj)
        lines.append(generate_method_get_child_object(obj))
        lines.append(generate_method_get_property(obj))

    # Contained children member variables
    lines.header += [
        '',
        [f'Contained{child.typename} {child.id};' for child in obj.contained_children],
        '};'
    ]

    lines.header += [
            '',
            f'class {obj.typename}: public Contained{obj.typename}',
            '{',
            'public:',
            [
                f'{obj.typename}({obj.database.typename}& db):',
                [', '.join([
                    f'Contained{obj.typename}(*{obj.store.typename}::open(db), {get_string(obj.path)})',
                    f'store({obj.store.typename}::open(db))'
                ])],
                '{',
                '}',
                '',
                'ConfigDB::Store& getStore() override',
                '{',
                ['return *store;'],
                '}',
            ],
            '',
            [f'std::shared_ptr<{obj.store.typename}> store;'],
            '};'
        ]

    return lines


def generate_item_object(obj: Object) -> CodeLines:
    '''Generate code for Array Item Object implementation'''

    lines = CodeLines(
        [
            '',
            f'class {obj.typename}: public ConfigDB::{obj.base_class}',
            '{',
            'public:',
            [f'using {obj.classname}::{obj.classname};'],
        ],
        []
    )

    # Append child object definitions
    for child in obj.children:
        lines.append(generate_object(child))

    lines.header += [[
        '',
        f'{obj.typename}({obj.store.typename}& store, const String& path):',
        [', '.join([
            f'{obj.classname}(store, path)',
            *(f'{child.id}(array.getStore())' for child in obj.contained_children)
        ])],
        '{',
        '}',
    ]]

    if isinstance(obj, Array):
        lines.append(generate_array_accessors(obj))
    else:
        lines.header += generate_object_accessors(obj)
        lines.append(generate_method_get_child_object(obj))
    lines.append(generate_method_get_property(obj))

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
            if item:
                if isinstance(item, str):
                    f.write(f'{indent}{item}\n')
                else:
                    dump_output(item, indent + '    ')
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
