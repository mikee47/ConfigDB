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

STRING_PREFIX = 'fstr_'

class StringTable:
    '''All string data stored as FSTR values
    This de-duplicates strings and returns a unique C++ identifier to be used.
    '''

    def __init__(self):
        self.keys = ['empty']
        self.values = ['']

    def __getitem__(self, value: str | None):
        if value is None:
            return 'nullptr'
        return STRING_PREFIX + self.keys[self.get_index(value)]

    def get_index(self, value: str) -> int:
        try:
            return self.values.index(value or '')
        except ValueError:
            pass
        ident = make_identifier(value)[:MAX_STRINGID_LEN]
        if not ident:
            ident = str(len(self.values))
        if ident in self.keys:
            i = 0
            while f'{ident}_{i}' in self.keys:
                i += 1
            ident = f'{ident}_{i}'
        i = len(self.keys)
        self.keys.append(ident)
        self.values.append(value)
        return i

    def items(self):
        return zip(self.keys, self.values)


strings = StringTable()

@dataclass
class IntRange:
    is_signed: bool
    bits: int
    minimum: int
    maximum: int

    @property
    def typemin(self):
        return -(2 ** (self.bits - 1)) if self.is_signed else 0

    @property
    def typemax(self):
        bits = self.bits
        if self.is_signed:
            bits -= 1
        return (2 ** bits) - 1


@dataclass
class Property:
    name: str
    index: int
    fields: dict

    @property
    def ptype(self):
        return self.fields['type']

    def get_intrange(self) -> IntRange | None:
        if self.ptype != 'integer':
            return None
        int32 = IntRange(True, 32, 0, 0)
        minval = self.fields.get('minimum', int32.typemin)
        maxval = self.fields.get('maximum', int32.typemax)
        r = IntRange(minval < 0, 8, minval, maxval)
        while minval < r.typemin or maxval > r.typemax:
            r.bits *= 2
        return r

    @property
    def ctype(self):
        if self.ptype != 'integer':
            return CPP_TYPENAMES[self.ptype]
        r = self.get_intrange()
        return f'int{r.bits}_t' if r.is_signed else f'uint{r.bits}_t'

    @property
    def ctype_constref(self):
        return 'const String&' if self.ptype == 'string' else self.ctype

    @property
    def ctype_struct(self):
        if self.ptype == 'string':
            return 'ConfigDB::StringId'
        return self.ctype

    @property
    def property_type(self):
        if self.ptype != 'integer':
            tag = self.ptype.capitalize()
        else:
            r = self.get_intrange()
            tag = f'Int{r.bits}' if r.is_signed else f'UInt{r.bits}'
        return 'PropertyType::' + tag

    @property
    def id(self):
        return make_identifier(self.name)

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
            return strings[default]
        if self.ptype == 'boolean':
            return 'true' if default else 'false'
        return str(default) if default else 0

    @property
    def default_structval(self):
        '''Value suitable for static initialisation {x}'''
        return make_comment(self.default) if self.ptype == 'string' else self.default_str


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
    def typename_contained(self):
        return self.typename if self.is_item else 'Contained' + self.typename

    @property
    def typename_struct(self):
        return self.typename_contained + '::Struct'

    @property
    def namespace(self):
        obj = self.parent
        ns = []
        while obj:
            if not isinstance(obj, (Store, ObjectArray)):
                ns.insert(0, obj.typename_contained)
            obj = obj.parent
        return '::'.join(ns)

    @property
    def id(self):
        return make_identifier(self.name) if self.name else 'root'

    @property
    def index(self):
        try:
            return self.parent.children.index(self)
        except ValueError:
            return 0

    @property
    def classname(self):
        return type(self).__name__

    @property
    def base_class(self):
        return f'{self.classname}'

    @property
    def is_root(self):
        return isinstance(self.parent, Store)

    @property
    def is_array(self):
        return False

    @property
    def is_item(self):
        return self.parent.is_array

    @property
    def is_item_member(self):
        return self.is_item or self.parent.is_item_member

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name


@dataclass
class Array(Object):
    items: Property = None

    @property
    def is_array(self):
        return True

    @property
    def base_class(self):
        return 'StringArray' if self.items.ptype == 'string' else 'Array'

    @property
    def typename_struct(self):
        return 'ConfigDB::ArrayId'


@dataclass
class ObjectArray(Object):
    items: Object = None

    @property
    def is_array(self):
        return True

    @property
    def typename_struct(self):
        return 'ConfigDB::ArrayId'


@dataclass
class Store(Object):
    store_ns: str = None

    @property
    def typename(self):
        return f'{super().typename}Store'

    @property
    def typename_contained(self):
        return self.typename

    @property
    def namespace(self):
        return self.database.typename

    @property
    def base_class(self):
        return f'{self.store_ns}::Store'


@dataclass
class Database(Object):
    @property
    def is_item_member(self):
        return False

    @property
    def typename_contained(self):
        return self.typename

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
    s = re.sub(r'[^A-Za-z0-9]+', '_', s)
    ident = ''
    for c in s:
        if c in ['-', '_']:
            up = True
        else:
            ident += c.upper() if up else c
            up = False
    return ident


def make_comment(s: str):
    '''Make a valid C++ comment containing the given string'''
    return '/* ' + s.replace('*/', '-/') + ' */'


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
    return '"' + s.replace('"', '\\"').encode('unicode-escape').decode('utf-8') + '"'


def make_static_initializer(entries: list, term_str: str = '') -> list:
    '''Create a static structured initialiser list from given items'''
    return [ '{', [str(e) + ',' for e in entries], '}' + term_str]


def declare_templated_class(obj: Object, tparams: list = None) -> list[str]:
    params = [f'{obj.typename_contained}']
    if tparams:
        params += tparams
    return [
        '',
        f'class {obj.typename_contained}: public ConfigDB::{obj.base_class}Template<{", ".join(params)}>',
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
    db.children.append(db.store)
    root = Object(db.store, '', db.store)
    db.store.children.append(root)

    def parse(obj: Object, properties: dict):
        obj_root = obj
        for key, value in properties.items():
            obj = obj_root
            # Filter out support property types
            prop = Property(key, len(obj.properties), value)
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
                db.children.append(store)
                obj = store
            else:
                store = obj.store
            if prop.ptype == 'object':
                child = Object(obj, key, store)
                parse(child, value['properties'])
            elif prop.ptype == 'array':
                items = value['items']
                if items['type'] == 'object':
                    arr = ObjectArray(obj, key, store)
                    arr.items = Object(arr, f'{arr.typename}Item', store)
                    parse(arr.items, items['properties'])
                else:
                    arr = Array(obj, key, store)
                    parse(arr, {'items': items})
                    assert len(arr.properties) == 1
                    arr.items = arr.properties[0]
                    arr.properties = []
                child = arr
            else:
                raise ValueError('Bad type ' + repr(prop))
            obj.children.append(child)

    parse(root, db.properties)
    return db


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''

    lines = CodeLines(
        [
            '#include <ConfigDB/Database.h>',
            '',
            f'class {db.typename}: public ConfigDB::DatabaseTemplate<{db.typename}>',
            '{',
            'public:',
            [
                'static const ConfigDB::DatabaseInfo typeinfo;',
                '',
                f'{db.typename}(const String& path): DatabaseTemplate(typeinfo, path)',
                '{',
                '}',
            ],
        ],
        [])

    for store in db.children:
        for obj in store.children:
            lines.append(generate_object(obj))

    lines.header += ['};']

    lines.source += [
        '',
        f'const DatabaseInfo {db.typename}::typeinfo PROGMEM {{',
        [
            f'{len(db.children)},',
            '{',
            [f'&{store.children[0].typename}::typeinfo,' for store in db.children],
            '}'
        ], '};'
    ]

    lines.source[:0] = [
        f'#include "{db.name}.h"',
        '',
        'namespace {',
        [f'DEFINE_FSTR({STRING_PREFIX}{id}, {make_string(value)})' for id, value in strings.items() if value],
        '} // namespace',
        '',
        'using namespace ConfigDB;',
    ]

    return lines


def generate_typeinfo(obj: Object) -> CodeLines:
    '''Generate type information'''

    lines = CodeLines([], [])
    if isinstance(obj, ObjectArray):
        objinfo = [obj.items]
    else:
        objinfo = obj.children
    if objinfo:
        lines.header += [
            'static const ConfigDB::ObjectInfo* objinfo[];'
        ]
        lines.source += [
            '',
            f'const ObjectInfo* {obj.namespace}::{obj.typename_contained}::objinfo[] PROGMEM {{',
            [f'&{ob.typename_contained}::typeinfo,' for ob in objinfo],
            '};'
        ]

    if isinstance(obj, Array):
        propinfo = [obj.items]
    else:
        propinfo = obj.properties

    lines.header += [
        'static const ConfigDB::ObjectInfo typeinfo;'
    ]
    lines.source += [
        '',
        f'constexpr const ObjectInfo {obj.namespace}::{obj.typename_contained}::typeinfo PROGMEM',
        '{',
        *([str(e) + ','] for e in [
            f'ObjectType::{"Store" if obj.is_root else obj.classname}',
            'fstr_empty' if obj.is_item else strings[obj.name],
            'nullptr' if obj.is_root or isinstance(obj, Store) else f'&{obj.parent.namespace}::{obj.parent.typename_contained}::typeinfo',
            'objinfo' if objinfo else 'nullptr',
            'nullptr' if obj.is_array or isinstance(obj, Store) else '&defaultData',
            0 if isinstance(obj, Store) else f'sizeof({obj.typename_struct})',
            len(objinfo),
            len(propinfo),
        ]),
        [
            '{',
            *(make_static_initializer([
                'fstr_empty' if obj.is_array else strings[prop.name],
                'nullptr' if prop.default is None or prop.ptype != 'string' else f'&{strings[prop.default]}',
                f'{prop.property_type}'
            ], ',') for prop in propinfo),
            '}',
        ],
        '};'
    ]

    return lines


def generate_object_struct(obj: Object) -> CodeLines:
    '''Generate struct definition for this object'''

    if obj.is_array:
        return CodeLines([], [])

    return CodeLines([
        '',
        'struct __attribute((packed)) Struct {',
        [
            'using Ptr = Struct*;',
            '',
            *(f'{child.typename_struct} {child.id}{{}};' for child in obj.children),
            *(f'{prop.ctype_struct} {prop.id}{{{prop.default_structval}}};' for prop in obj.properties)
        ],
        '};',
        '',
        'static const Struct defaultData;',
    ],
    [
        '',
        f'const {obj.namespace}::{obj.typename_contained}::Struct PROGMEM {obj.namespace}::{obj.typename_contained}::defaultData;'
    ])


def generate_property_accessors(obj: Object) -> list:
    '''Generate typed get/set methods for each property'''

    return [*((
        '',
        f'{prop.ctype} get{prop.typename}() const',
        '{',
        ['return ' + ('getString(' if prop.ptype == 'string' else '(') + f'Struct::Ptr(data)->{prop.id});'],
        '}',
        '',
        f'void set{prop.typename}({prop.ctype_constref} value)',
        '{',
        [f'Struct::Ptr(data)->{prop.id} = ' + ('getStringId(value);' if prop.ptype == 'string' else 'value;')],
        '}'
        ) for prop in obj.properties),
    ]


def generate_object(obj: Object) -> CodeLines:
    '''Generate code for Object implementation'''

    typeinfo = generate_typeinfo(obj)

    if isinstance(obj, ObjectArray):
        item_lines = generate_item_object(obj.items)
        return CodeLines(
            [
                *item_lines.header,
                *declare_templated_class(obj, [obj.items.typename]),
                typeinfo.header,
                generate_contained_constructors(obj),
                '};',
                *generate_outer_class(obj)
            ],
            item_lines.source + typeinfo.source)

    if isinstance(obj, Array):
        return CodeLines(
            [
                *declare_templated_class(obj, [obj.items.ctype]),
                typeinfo.header,
                generate_contained_constructors(obj),
                '};',
                *generate_outer_class(obj)
            ],
            typeinfo.source)

    lines = CodeLines(
        [
            *declare_templated_class(obj),
            typeinfo.header,
        ],
        [])

    # Append child object definitions
    for child in obj.children:
        lines.append(generate_object(child))

    struct = generate_object_struct(obj)
    lines.header += [
        struct.header,
        generate_contained_constructors(obj),
    ]
    lines.source += struct.source + typeinfo.source

    lines.header += generate_property_accessors(obj)

    # Contained children member variables
    if obj.children:
        lines.header += [
            '',
            'public:',
            [f'{child.typename_contained} {child.id};' for child in obj.children],
        ]

    lines.header += [
        '};',
        *generate_outer_class(obj)
    ]

    return lines


def generate_contained_constructors(obj: Object) -> list:
    headers = []
    if not obj.is_item_member:
        headers = [
            '',
            f'{obj.typename_contained}(ConfigDB::Store& store): ' + ', '.join([
                f'{obj.base_class}Template(store)',
                *(f'{child.id}(*this, Struct::Ptr(data)->{child.id})' for child in obj.children)
            ]),
            '{',
            '}',
        ]

    if not obj.is_root:
        data_type = 'ConfigDB::ArrayId' if obj.is_array else 'Struct'
        headers += [
            '',
            f'{obj.typename_contained}({obj.parent.typename_contained}& parent, {data_type}& data): ' + ', '.join([
                f'{obj.base_class}Template(parent, &data)',
                *(f'{child.id}(*this, data.{child.id})' for child in obj.children)
            ]),
            '{',
            '}',
        ]

    return headers


def generate_outer_class(obj: Object) -> list:
    return [] if obj.is_item_member else [
        '',
        f'class {obj.typename}: public {obj.typename_contained}',
        '{',
        'public:',
        [
            f'{obj.typename}(std::shared_ptr<ConfigDB::Store> store): {obj.typename_contained}(*store), store(store)',
            '{',
            '}',
            '',
            f'{obj.typename}(ConfigDB::Database& db): {obj.typename}(db.openStore({obj.store.children[0].typename}::typeinfo))',
            '{',
            '}',
        ],
        '',
        'private:',
        [f'std::shared_ptr<ConfigDB::Store> store;'],
        '};'
    ]


def generate_item_object(obj: Object) -> CodeLines:
    '''Generate code for Array Item Object implementation'''

    lines = CodeLines(declare_templated_class(obj), [])

    # Append child object definitions
    for child in obj.children:
        lines.append(generate_object(child))

    struct = generate_object_struct(obj)
    typeinfo = generate_typeinfo(obj)
    lines.header += [[
        '',
        *struct.header,
        *typeinfo.header,
        '',
        f'{obj.typename}(ConfigDB::{obj.parent.base_class}& {obj.parent.id}, Struct& data):',
        [', '.join([
            f'{obj.classname}Template({obj.parent.id}, &data)',
            *(f'{child.id}(*this, data.{child.id})' for child in obj.children)
        ])],
        '{',
        '}',
    ]]
    lines.source += struct.source + typeinfo.source

    if isinstance(obj, Array):
        lines.header += generate_array_accessors(obj)
    else:
        lines.header += generate_property_accessors(obj)

    # Contained children member variables
    if obj.children:
        lines.header += [
            '',
            'public:',
            [f'{child.typename_contained} {child.id};' for child in obj.children],
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
