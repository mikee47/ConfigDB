'''ConfigDB database generator'''

from __future__ import annotations
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
    'union': '-',
    'string': 'String',
    'integer': 'int',
    'boolean': 'bool',
    'number': 'ConfigDB::Number',
}

STRING_ID_SIZE = 2
ARRAY_ID_SIZE = 2
UNION_TAG_SIZE = 1
UNION_TAG_TYPE = 'uint8_t'
# These values get truncated by ConstNumber during C++ compilation
NUMBER_MIN = -1e100
NUMBER_MAX = 1e100

CPP_TYPESIZES = {
    'bool': 1,
    'int8_t': 1,
    'int16_t': 2,
    'int32_t': 4,
    'int64_t': 8,
    'uint8_t': 1,
    'uint16_t': 2,
    'uint32_t': 4,
    'uint64_t': 8,
    'ConfigDB::Number': 4,
    'String': STRING_ID_SIZE,
}

STRING_PREFIX = 'fstr_'

schemas: dict[str, dict] = dict()

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
        return STRING_PREFIX + self.keys[self.get_index(str(value))]

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


@dataclass
class Range:
    minimum: int | float
    maximum: int | float

    def check(self, value: int | float | list):
        if isinstance(value, list):
            for v in value:
                self.check(v)
            return
        if self.minimum <= value <= self.maximum:
            return
        raise ValueError(f'Value {value} outside range {self.minimum} <= value <= {self.maximum}')


@dataclass
class IntRange(Range):
    is_signed: bool
    bits: int

    @staticmethod
    def deduce(minval: int, maxval: int) -> IntRange:
        r = IntRange(minval, maxval, minval < 0, 8)
        while minval < r.typemin or maxval > r.typemax:
            r.bits *= 2
        return r

    @property
    def typemin(self):
        return -(2 ** (self.bits - 1)) if self.is_signed else 0

    @property
    def typemax(self):
        bits = self.bits
        if self.is_signed:
            bits -= 1
        return (2 ** bits) - 1

    @property
    def ctype(self):
        return f'int{self.bits}_t' if self.is_signed else f'uint{self.bits}_t'

    @property
    def property_type(self):
        return f'Int{self.bits}' if self.is_signed else f'UInt{self.bits}'


def get_ptype(fields: dict):
    '''Identify a pseudo-type 'union' which makes script a bit clearer'''
    return 'union' if 'oneOf' in fields else fields['type']


@dataclass
class Property:
    parent: 'Object'
    name: str
    ref: dict
    ptype: str
    ctype: str
    ctype_override: str
    property_type: str
    default: str | int
    alias: str | list[str] | None
    enum: list
    enum_type: str = None
    enum_ctype: str = None
    intrange: IntRange = None
    numrange: Range = None

    def __init__(self, parent: Object, key: str, fields: dict):
        def error(msg: str):
            raise ValueError(f'"{parent.path}": {msg} for "{self.name}"')

        self.parent = parent
        self.name = key
        self.ptype = get_ptype(fields)
        self.ref = fields.get('ref')
        self.ctype_override = fields.get('ctype')
        self.default = fields.get('default')
        self.ctype = CPP_TYPENAMES[self.ptype]
        self.property_type = self.ptype.capitalize()
        self.alias = fields.get('alias')

        if self.ptype == 'integer':
            int32 = IntRange(0, 0, True, 32)
            minval = fields.get('minimum', int32.typemin)
            maxval = fields.get('maximum', int32.typemax)
            self.intrange = r = IntRange.deduce(minval, maxval)
            r.check(self.default or 0)
            self.ctype = r.ctype
            self.property_type = r.property_type
        elif self.ptype == 'number':
            minval = fields.get('minimum', NUMBER_MIN)
            maxval = fields.get('maximum', NUMBER_MAX)
            self.numrange = r = Range(minval, maxval)
            r.check(self.default or 0)

        if not self.ctype:
            error(f'Invalid property type "{self.ptype}"')

        self.enum = fields.get('enum')
        if self.enum:
            if 'minimum' in fields or 'maximum' in fields:
                error('enum and minimum/maximum fields are mutually-exclusive')
            self.enum_type = self.property_type
            self.enum_ctype = self.ctype
            if self.ptype == 'string':
                pass
            elif self.ptype == 'integer':
                minval, maxval = min(self.enum), max(self.enum)
                self.intrange = r = IntRange.deduce(minval, maxval)
                r.check(self.enum)
                self.enum_type = r.property_type
                self.enum_ctype = r.ctype
            elif self.ptype == 'number':
                self.numrange = r = Range(NUMBER_MIN, NUMBER_MAX)
                r.check(self.enum)
                self.enum_ctype = 'const_number_t'
            else:
                error(f'Unsupported enum type "{self.ptype}"')
            if self.default:
                if self.default not in self.enum:
                    error(f'default "{self.default}" not in enum')
                self.default = self.enum.index(self.default)
            else:
                self.default = 0
            self.ptype = 'integer'
            self.property_type = 'Enum'
            self.ctype = 'uint8_t'

    @property
    def ctype_ret(self):
        '''Type to use for accessor return value'''
        return self.ctype_override or self.ctype

    @property
    def propdata_id(self):
        return 'uint8' if self.enum else self.property_type.lower()

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage type'''
        return CPP_TYPESIZES[self.ctype]

    @property
    def id(self):
        return make_identifier(self.name)

    @property
    def typename(self):
        return make_typename(self.name)

    @property
    def default_str(self):
        default = self.default
        if self.ptype == 'string':
            return self.parent.strings[default]
        if self.ptype == 'boolean':
            return 'true' if default else 'false'
        if self.ptype == 'number':
            return f'const_number_t({self.default})' if self.default else '0'
        return str(default) if default else 0


@dataclass
class Object:
    parent: 'Object'
    name: str
    ref: str | None
    alias: str | list[str] | None
    is_store: bool = False
    schema_id: str = None
    properties: list[Property] = field(default_factory=list)
    children: list['Object'] = field(default_factory=list)

    @property
    def store(self):
        return self if self.is_store else self.parent.store

    @property
    def database(self):
        return self.parent.database

    @property
    def typename(self):
        return make_typename(self.ref or self.name or 'Root')

    @property
    def typename_contained(self):
        return 'Contained' + self.typename

    @property
    def typename_outer(self):
        return make_typename(self.name or 'Root')

    @property
    def typename_updater(self):
        return f'{self.typename}Updater'

    @property
    def typename_struct(self):
        return self.typename_contained + '::Struct'

    def get_offset(self, obj: Object):
        '''Offset of a child object in the data structure'''
        offset = 0
        for c in self.children:
            if c is obj:
                return offset
            offset += c.data_size
        assert False, 'Not a child'

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return sum(child.data_size for child in self.children) + sum(prop.data_size for prop in self.properties)

    @property
    def has_struct(self):
        return self.children or self.properties

    @property
    def index(self):
        return self.parent.children.index(self)

    @property
    def namespace(self):
        if self.ref or (self.is_item and self.parent.ref):
            return self.store.parent.typename
        obj = self.parent
        ns = []
        while obj:
            if not isinstance(obj, ObjectArray):
                ns.insert(0, obj.typename_contained)
            obj = obj.database if obj.ref else obj.parent
        return '::'.join(ns)

    @property
    def id(self):
        return make_identifier(self.name) if self.name else 'root'

    @property
    def classname(self):
        return type(self).__name__

    @property
    def base_class(self):
        return f'{self.classname}'

    @property
    def is_root(self):
        return self in self.database.children

    @property
    def is_array(self):
        return False

    @property
    def is_union(self):
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
    default: list = None

    @property
    def is_array(self):
        return True

    @property
    def base_class(self):
        return 'StringArray' if self.items.ptype == 'string' else 'Array'

    @property
    def typename_struct(self):
        return 'ConfigDB::ArrayId'

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return ARRAY_ID_SIZE


@dataclass
class ObjectArray(Object):
    items: Object = None
    default: list = None

    @property
    def is_array(self):
        return True

    @property
    def typename_struct(self):
        return 'ConfigDB::ArrayId'

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return ARRAY_ID_SIZE


@dataclass
class Union(Object):
    @property
    def is_union(self):
        return True

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return max(child.data_size for child in self.children) + UNION_TAG_SIZE


@dataclass
class Database(Object):
    strings: StringTable = StringTable()
    object_defs: dict[Object] = field(default_factory=dict)
    forward_decls: set[str] = None

    @property
    def database(self):
        return self

    @property
    def is_item_member(self):
        return False

    @property
    def typename_contained(self):
        return self.typename

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


def resolve_ref(parent: Object, fields: dict, db: Database) -> tuple[str, dict]:
    '''If property contains a reference, deal with it'''
    print("RESOLVE", parent.schema_id, fields)
    ref = fields.get('$ref')
    if not ref:
        return None, fields
    prefix = '#/$defs/'
    resolved_fields = None
    if ref.startswith('#/'):
        ref = ref[2:]
        schema_id = parent.schema_id or db.schema_id
        print("INT", schema_id)
    else:
        schema_id, _, ref = ref.partition('/')
        print("EXT", schema_id)
    x = schemas[schema_id]
    print("REF", ref, x)
    while ref:
        node, _, ref = ref.partition('/')
        print("X", node, ref)
        x = x[node]
    resolved_fields = x

    #     # Internal reference
    #     ref = ref[len(prefix):]
    #     schema_id = parent.schema_id
    #     if schema_id:
    #         definitions = schemas[schema_id]
    #         print('EXT', schema_id, ref, definitions)
    #     else:
    #         definitions = db.definitions
    #         print('INT', ref, definitions)
    # else:
    #     # External reference
    #     # TODO: This is very simple. Should revise this to support proper URI's as per spec.
    #     schema_id, _, ref = ref.partition('/')
    #     definitions = schemas[schema_id]['$defs']
    #     print('EXT', schema_id, ref, definitions)

    # if not definitions:
    #     raise ValueError('Schema missing definitions')
    # resolved_fields = definitions.get(ref)
    if resolved_fields is None:
        raise ValueError('Cannot resolve ' + ref)
    resolved_fields['schema_id'] = schema_id

    return ref, resolved_fields


def resolve_prop_ref(parent: Object, fields: dict, db: Database) -> None:
    '''Resolve reference and update fields'''
    ref, resolved_fields = resolve_ref(parent, fields, db)
    if ref is None:
        return

    fields['ref'] = ref

    # Object definitions are used as-is
    if get_ptype(resolved_fields) in ['object', 'union']:
        fields.update(resolved_fields)
        return

    # For simple properties the definition is a template,
    # so copy over any non-existent values
    for key, value in resolved_fields.items():
        if key not in fields:
            fields[key] = value;


def parse_schema(schema_id: str, config: dict) -> Database:
    '''Validate and parse schema into python objects'''
    database = Database(None, schema_id, None, None, properties=config['properties'], schema_id=schema_id)
    database.include = config.get('include', [])
    root = Object(database, '', None, None, is_store=True)
    database.children.append(root)

    def parse_properties(parent: Object, properties: dict):
        for key, fields in properties.items():
            resolve_prop_ref(parent, fields, database)
            prop = Property(parent, key, fields)
            if prop.ctype != '-': # object or array
                parent.properties.append(prop)
                continue

            # Objects with 'store' annoation are managed by database, otherwise they live in root object
            if 'store' in fields:
                if parent is not root:
                    raise ValueError(f'{key} cannot have "store" annotation, not a root object')
                obj = database
            else:
                obj = parent

            child = parse_object(obj, key, fields)
            if child:
                obj.children.append(child)

    def parse_object(parent: Object, key: str, fields: dict) -> Object:
        print("parse_object", parent.name, key, fields)
        ref, fields = resolve_ref(parent, fields, database)
        prop_type = get_ptype(fields)

        def createObject(Class):
            return Class(parent, key, ref, fields.get('alias'),
                is_store = ('store' in fields),
                schema_id = fields.get('schema_id'))

        if prop_type == 'object':
            if 'default' in fields:
                raise ValueError('Object default not supported (use default on properties)')
            obj = createObject(Object)
            database.object_defs[obj.typename] = obj
            parse_properties(obj, fields.get('properties', {}))
            return obj

        if prop_type == 'array':
            item_ref, items = resolve_ref(parent, fields['items'], database)
            items_type = get_ptype(items)
            if items_type in ['object', 'union']:
                if 'default' in fields:
                    raise ValueError('ObjectArray default not supported')
                arr = createObject(ObjectArray)
                database.object_defs[arr.typename] = arr
                arr.items = database.object_defs.get(item_ref)
                if arr.items and arr.items.ref:
                    return arr
                arr.items = Object(arr, item_ref or f'{arr.typename}Item', None, None)
                arr.items.ref = item_ref
                database.object_defs[arr.items.typename] = arr.items
                if items_type == 'union':
                    parse_object(arr.items, item_ref, items)
                else:
                    parse_properties(arr.items, items['properties'])
                return arr

            # Simple array
            arr = createObject(Array)
            parse_properties(arr, {'items': items})
            assert len(arr.properties) == 1
            arr.items = arr.properties[0]
            arr.properties = []
            arr.default = fields.get('default')
            return arr

        if prop_type == 'union':
            if 'default' in fields:
                raise ValueError('Union default not supported')
            union = createObject(Union)
            database.object_defs[union.typename] = union
            for opt in fields['oneOf']:
                ref, opt = resolve_ref(union, opt, database)
                choice = database.object_defs.get(ref)
                if not choice:
                    choice = parse_object(union, ref, opt)
                    choice.ref = ref
                    database.object_defs[choice.typename] = choice
                union.children.append(choice)
            return union

        raise ValueError('Bad type ' + prop_type)

    parse_properties(root, database.properties)
    return database


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''

    db.forward_decls = {
        'ContainedRoot',
        'RootUpdater'
    }
    external_defs = []
    for obj in db.object_defs.values():
        if obj.ref:
            if obj.schema_id:
                db.include.append(f'{obj.schema_id}.h')
                ns = make_typename(obj.schema_id)
                external_defs += [
                    f'using {obj.typename_contained} = {ns}::{obj.typename_contained};',
                    f'using {obj.typename_updater} = {ns}::{obj.typename_updater};',
                ]
            else:
                db.forward_decls |= {obj.typename_contained, obj.typename_updater}

    lines = CodeLines(
        [
            '/*',
            generate_structure(db),
            '*/',
            '#include <ConfigDB/Database.h>',
            *(f'#include <{file}>' for file in db.include),
            '',
            f'class {db.typename}: public ConfigDB::DatabaseTemplate<{db.typename}>',
            '{',
            'public:',
            [
                *external_defs,
                *(f'class {name};' for name in db.forward_decls),
                '',
                'static const ConfigDB::DatabaseInfo typeinfo;',
                '',
                'using DatabaseTemplate::DatabaseTemplate;',
                '',
                '/*',
                ' * Contained classes are reference objects only, and do not contain the actual data.',
                ' */'
            ]
        ],
        [
            '',
            f'const DatabaseInfo {db.typename}::typeinfo PROGMEM {{',
            [
                f'{db.strings[db.name]},',
                f'{len(db.children)},',
                '{',
                *(make_static_initializer(
                    [
                    '.type = PropertyType::Object',
                    f'.name = {db.strings[store.name]}',
                    '.offset = 0',
                    f'.variant = {{.object = &{store.typename_contained}::typeinfo}}'
                    ], ',') for store in db.children),
                '}'
            ],
            '};'
        ])

    for obj in reversed(db.object_defs.values()):
        if obj.ref:
            if obj.schema_id:
                pass
            else:
                lines.append(generate_object(obj))

    for store in db.children:
        if not store.ref:
            lines.append(generate_object(store))

    lines.header += [
        '',
        '',
        [
            '/*',
            ' * Outer classes contain a shared store pointer plus contained classes to access that data.',
            ' * Application code instantiate these directly.',
            ' */'
        ]
    ]
    def generate_outer_class(obj: Object, store_offset: int) -> list:
        template = f'ConfigDB::OuterObjectTemplate<{obj.typename_contained}, {obj.typename_updater}, {obj.database.typename}, {obj.store.index}, {obj.parent.typename_contained}, {obj.index}, {store_offset}>'
        if not obj.is_store:
            store_offset += obj.parent.get_offset(obj)
        return [
            '',
            f'class {obj.typename_outer}: public {template}',
            '{',
            'public:',
            [
                'using OuterObjectTemplate::OuterObjectTemplate;',
            ],
            *[generate_outer_class(child, store_offset) for child in obj.children if not obj.is_item],
            '};'
        ] if obj.children and not obj.is_union else [
            f'using {obj.typename_outer} = {template};'
        ]
    for store in db.children:
        lines.header.append(generate_outer_class(store, 0))

    lines.header += ['};']

    for obj in db.object_defs.values():
        if not obj.is_union:
            continue
        decl = f'String toString({obj.namespace}::{obj.typename_contained}::Tag tag)'
        lines.header += [
            '',
            f'{decl};'
        ]
        lines.source += [
            '',
            decl,
            '{',
            [
                'switch(unsigned(tag)) {',
                [f'case {index}: return {db.strings[child.name]};' for index, child in enumerate(obj.children)],
                ['default: return nullptr;'],
                '}'
            ],
            '}'
        ]


    # Insert this at end once string table has been populated
    lines.source[:0] = [
        f'#include "{db.name}.h"',
        '',
        'namespace {',
        [f'DEFINE_FSTR({STRING_PREFIX}{id}, {make_string(value)})' for id, value in db.strings.items() if value],
        '} // namespace',
        '',
        'using namespace ConfigDB;',
        '',
        '#ifdef __clang__',
        '#pragma GCC diagnostic ignored "-Wc99-designator"',
        '#endif'
    ]

    return lines


def generate_structure(db: Database) -> list[str]:
    structure = []
    def print_structure(obj: Object, indent: int, offset: int):
        def add(offset: int, id: str, typename: str):
            structure.append(f'{offset:4} {"".ljust(indent*3)} {id}: {typename}')
        add(offset, obj.id, obj.base_class)
        for c in obj.children:
            print_structure(c, indent+1, offset)
            if not obj.is_union:
                offset += c.data_size
        indent += 1
        for prop in obj.properties:
            add(offset, prop.id, prop.ctype)
            if not obj.is_union:
                offset += prop.data_size
        if obj.is_union:
            add(offset + obj.data_size - UNION_TAG_SIZE, 'tag', 'uint8_t')
    for store in db.children:
        print_structure(store, 0, 0)
        structure += ['']
    return structure


def declare_templated_class(obj: Object, tparams: list = None, is_updater: bool = False) -> list[str]:
    typename = obj.typename_updater if is_updater else obj.typename_contained
    template = 'UpdaterTemplate' if is_updater else 'Template'
    params = [f'{obj.typename_contained}']
    if is_updater:
        params.insert(0, typename)
    if tparams:
        params += tparams
    return [
        '',
        f'class {typename}: public ConfigDB::{obj.base_class}{template}<{", ".join(params)}>',
        '{',
        'public:',
    ]


def generate_typeinfo(obj: Object) -> CodeLines:
    '''Generate type information'''

    db = obj.database

    lines = CodeLines(
        ['static const ConfigDB::ObjectInfo typeinfo;']
    )

    def getVariantInfo(prop: Property) -> list:
        if prop.enum:
            values = prop.enum
            obj_type = f'{obj.namespace}::{obj.typename_contained}'
            item_type = 'const FSTR::String*' if prop.enum_type == 'String' else prop.enum_ctype
            if obj.is_array and prop.ctype_override:
                lines.header += [
                    '',
                    f'using Item = {prop.ctype_override};'
                ]
            lines.header += [
                '',
                'struct ItemType {',
                [
                    'ConfigDB::EnumInfo enuminfo;',
                    f'{item_type} data[{len(values)}];',
                    '',
                    *([
                        'const FSTR::Vector<FSTR::String>& values() const',
                        '{',
                        ['return enuminfo.getStrings();'],
                        '}',
                    ] if prop.enum_type == "String" else
                    [
                        f'const FSTR::Array<{prop.enum_ctype}>& values() const',
                        '{',
                        [f'return enuminfo.getArray<{prop.enum_ctype}>();'],
                        '}',
                    ])
                ],
                '};',
                '',
                'static const ItemType itemtype;',
            ]
            lines.source += [
                '',
                f'constexpr const {obj_type}::ItemType {obj_type}::itemtype PROGMEM = {{',
                [
                    f'{{PropertyType::{prop.enum_type}, {{{len(values)} * sizeof({item_type})}}}},',
                    '{',
                    [f'&{db.strings[x]},' for x in values] if prop.enum_type == 'String' else
                    [f'{x},' for x in values],
                    '}'
                ],
                '};'
            ]
            return '.enuminfo = &itemtype.enuminfo'
        if prop.ptype == 'string':
            return f'.defaultString = &{db.strings[str(prop.default)]}' if prop.default else ''
        if prop.ptype == 'number':
            r = prop.numrange
            return f'.number = {{.minimum = {r.minimum}, .maximum = {r.maximum}}}'
        if prop.ptype == 'integer':
            r = prop.intrange
            if r.bits > 32:
                id = prop.id
                lines.source += [f'constexpr const {prop.ctype} PROGMEM {id}_minimum = {r.minimum};']
                lines.source += [f'constexpr const {prop.ctype} PROGMEM {id}_maximum = {r.maximum};']
                tag = 'int64' if r.is_signed else 'uint64'
                return f'.{tag} = {{.minimum = &{id}_minimum, .maximum = &{id}_maximum}}'
            tag = 'int32' if r.is_signed else 'uint32'
            return f'.{tag} = {{.minimum = {r.minimum}, .maximum = {r.maximum}}}'
        return ''

    proplist = []
    aliaslist = []

    def add_alias(alias: str | list):
        if alias is None:
            return
        if isinstance(alias, str):
            aliaslist.append([
                f'.type = PropertyType::Alias',
                f'.name = {db.strings[alias]}',
                f'.offset = {len(proplist) - 1}'
            ])
        if isinstance(alias, list):
            for x in alias:
                add_alias(x)

    offset = 0

    objinfo = [obj.items] if isinstance(obj, ObjectArray) else obj.children
    for child in objinfo:
        proplist += [[
            '.type = PropertyType::Object',
            '.name = ' + ('fstr_empty' if obj.is_array else db.strings[child.name]),
            f'.offset = {offset}',
            f'.variant = {{.object = &{child.namespace}::{child.typename_contained}::typeinfo}}'
        ]]
        add_alias(child.alias)
        if not obj.is_union:
            offset += child.data_size

    if obj.is_union:
        proplist += [[
            '.type = PropertyType::UInt8',
            f'.name = {db.strings['tag']}',
            f'.offset = {obj.data_size - UNION_TAG_SIZE}',
        ]]

    propinfo = [obj.items] if isinstance(obj, Array) else obj.properties
    for prop in propinfo:
        proplist += [[
            f'.type = PropertyType::{prop.property_type}',
            '.name = ' + ('fstr_empty' if obj.is_array else db.strings[prop.name]),
            f'.offset = {offset}',
            '.variant = {' + getVariantInfo(prop) + '}'
        ]]
        add_alias(prop.alias)
        offset += prop.data_size

    # Generate array default data
    defaultData = 'nullptr'
    if isinstance(obj, Array):
        arr: Array = obj
        if arr.default:
            id = f'{obj.typename_contained}_defaultData'
            defaultData = f'&{id}'
            if arr.items.enum:
                lines.source += [
                    '',
                    f'DEFINE_FSTR_ARRAY_LOCAL({id}, uint8_t,',
                    [f'{arr.items.enum.index(v)},' for v in arr.default],
                    ')'
                ]
            elif arr.items.ptype == 'string':
                lines.source += [
                    '',
                    f'DEFINE_FSTR_VECTOR_LOCAL({id}, FSTR::String,',
                    [f'&{db.strings[s]},' for s in arr.default],
                    ')'
                ]
            else:
                lines.source += [
                    '',
                    f'DEFINE_FSTR_ARRAY_LOCAL({id}, {arr.items.ctype},',
                    [f'{v},' for v in arr.default],
                    ')'
                ]
    elif obj.has_struct:
        defaultData = '&defaultData'

    lines.source += [
        '',
        f'const ObjectInfo {obj.namespace}::{obj.typename_contained}::typeinfo PROGMEM',
        '{',
        *([str(e) + ','] for e in [
            f'.type = ObjectType::{obj.classname}',
            f'.defaultData = {defaultData}',
            '.structSize = ' + ('sizeof(ArrayId)' if obj.is_array else 'sizeof(Struct)' if obj.has_struct else '0'),
            f'.objectCount = {len(objinfo)}',
            f'.propertyCount = {len(proplist) - len(objinfo)}',
            f'.aliasCount = {len(aliaslist)}'
        ]),
        [
            '.propinfo = {',
            *(make_static_initializer(prop, ',') for prop in proplist + aliaslist),
            '}'
        ],
        '};'
    ]

    return lines


def generate_object_struct(obj: Object) -> CodeLines:
    '''Generate struct definition for this object'''

    if obj.data_size == 0:
        return CodeLines()

    def get_ctype(prop):
        return 'ConfigDB::StringId' if prop.ptype == 'string' else prop.ctype

    def get_default(prop):
        if prop.ptype == 'string':
            return make_comment(prop.default) if prop.default else ''
        return prop.default_str

    children = [child for child in obj.children if child.data_size]

    if obj.is_union:
        body = [
            'union __attribute__((packed)) {',
            [f'{child.typename_struct} {child.id}{"{}" if index == 0 else ""};' for index, child in enumerate(children)],
            '};',
            f'{UNION_TAG_TYPE} tag{{0}};',
        ]
    else:
        body = [
            [f'{child.typename_struct} {child.id}{{}};' for child in children] if children else None,
            [f'{get_ctype(prop)} {prop.id}{{{get_default(prop)}}};' for prop in obj.properties]
        ]

    return CodeLines([
        '',
        'struct __attribute__((packed)) Struct {',
        body,
        '};',
        '',
        'static const Struct defaultData;',
    ],
    [
        '',
        f'const {obj.namespace}::{obj.typename_contained}::Struct PROGMEM {obj.namespace}::{obj.typename_contained}::defaultData{{}};'
    ])


def generate_property_accessors(obj: Object) -> list:
    '''Generate typed get/set methods for each property'''

    if obj.is_union:
        return [
            '',
            [
                'enum class Tag {',
                [f'{child.typename},' for child in obj.children],
                '};',
                '',
                'Tag getTag() const',
                '{',
                ['return Tag(Union::getTag());'],
                '}'
            ],
            *((
                '',
                f'const {child.typename_contained} as{child.typename}() const',
                '{',
                [f'return Union::as<{child.typename_contained}>({index});'],
                '}',
                ) for index, child in enumerate(obj.children))
            ]

    return [*((
        '',
        f'{prop.ctype_ret} get{prop.typename}() const',
        '{',
        [f'return getPropertyString({index});']
        if prop.ptype == 'string' else
        [f'return {prop.ctype_ret}(getPropertyData({index})->{prop.propdata_id});'],
        '}',
        ) for index, prop in enumerate(obj.properties))]


def generate_property_write_accessors(obj: Object) -> list:
    '''Generate typed get/set methods for each property'''

    def get_value_expr(prop: Property) -> str:
        if prop.ptype == 'string':
            stype = prop.ctype_ret
            return 'value' if stype == 'String' else f'String(value)'
        return '&value'

    def get_ctype(prop):
        if prop.ctype_override:
            return prop.ctype_override if prop.enum else f'const {prop.ctype_override}&'
        return 'const String&' if prop.ptype == 'string' else prop.ctype

    if obj.is_union:
        return [
            [
                '',
                'void setTag(Tag tag)',
                '{',
                ['Union::setTag(unsigned(tag));'],
                '}',
            ],
            *((
                '',
                f'{child.typename_updater} as{child.typename}()',
                '{',
                [f'return Union::as<{child.typename_updater}>({index});'],
                '}',
                '',
                f'{child.typename_updater} to{child.typename}()',
                '{',
                [f'return Union::to<{child.typename_updater}>({index});'],
                '}',
                ) for index, child in enumerate(obj.children))
        ]

    return [*((
        '',
        f'void set{prop.typename}({get_ctype(prop)} value)',
        '{',
        [f'setPropertyValue({index}, {get_value_expr(prop)});'],
        '}',
        '',
        f'void reset{prop.typename}()',
        '{',
        [f'setPropertyValue({index}, nullptr);'],
        '}'
        ) for index, prop in enumerate(obj.properties))]


def generate_object(obj: Object) -> CodeLines:
    '''Generate code for Object implementation'''

    typeinfo = generate_typeinfo(obj)
    constructors = generate_contained_constructors(obj)
    updater = generate_updater(obj)
    forward_decls = [] if obj.typename_updater in obj.database.forward_decls else [
        '',
        f'class {obj.typename_updater};'
    ]

    def generate_enum_class(properties: list[Property]):
        enumlist = []
        for prop in properties:
            if prop.enum and prop.ctype_override:
                tag_prefix = '' if prop.enum_type == 'String' else  'N'
                enumlist += [
                    '',
                    f'enum class {prop.ctype_override}: uint8_t {{',
                    [f'{tag_prefix}{make_identifier(str(x))},' for x in prop.enum],
                    '};'
                ]
        return enumlist

    if isinstance(obj, ObjectArray):
        item_lines = CodeLines() if obj.items.ref else generate_object(obj.items)
        return CodeLines(
            [
                *forward_decls,
                *item_lines.header,
                *declare_templated_class(obj, [obj.items.typename_contained]),
                typeinfo.header,
                constructors,
                '};',
                *updater,
            ],
            item_lines.source + typeinfo.source)

    if isinstance(obj, Array):
        return CodeLines(
            [
                *forward_decls,
                *generate_enum_class([obj.items]),
                *declare_templated_class(obj, [obj.items.ctype_ret]),
                typeinfo.header,
                constructors,
                '};',
                *updater,
            ],
            typeinfo.source)

    lines = CodeLines(
        [
            *forward_decls,
            *generate_enum_class(obj.properties),
            *declare_templated_class(obj),
            [f'using Updater = {obj.typename_updater};'],
            typeinfo.header,
        ])

    # Append child object definitions
    for child in obj.children:
        if not child.ref:
            lines.append(generate_object(child))

    lines.append(generate_object_struct(obj))
    lines.header += [
        constructors,
        *generate_property_accessors(obj)
    ]
    lines.source += typeinfo.source

    # Contained children member variables
    if obj.children and not obj.is_union:
        lines.header += [
            '',
            [f'const {child.typename_contained} {child.id};' for child in obj.children],
        ]

    lines.header += ['};']
    lines.header += updater

    return lines


def generate_updater(obj: Object) -> list:
    '''Generate code for Object Updater implementation'''

    constructors = generate_contained_constructors(obj, True)

    if isinstance(obj, ObjectArray):
        union_array_methods = [
            [
                '',
                f'{item.typename_updater} addItem{item.typename}()',
                '{',
                [f'return ObjectArray::addItem<ConfigDB::Union>().to<{item.typename_updater}>({tag});'],
                '}',
                '',
                f'{item.typename_updater} insertItem{item.typename}(unsigned index)',
                '{',
                [f'return ObjectArray::insertItem<ConfigDB::Union>(index).to<{item.typename_updater}>({tag});'],
                '}',
            ] for tag, item in enumerate(obj.items.children)
        ] if isinstance(obj.items, Union) else []
        return [
            *declare_templated_class(obj, [obj.items.typename_updater], True),
            constructors,
            *union_array_methods,
            '};',
        ]

    if isinstance(obj, Array):
        return [
            *declare_templated_class(obj, [obj.items.ctype_ret], True),
            constructors,
            '};',
        ]

    return [
        *declare_templated_class(obj, [], True),
        constructors,
        *generate_property_write_accessors(obj),
        None if obj.is_union else [
            '',
            *(f'{child.typename_updater} {child.id};' for child in obj.children)
        ],
        '};'
    ]


def generate_contained_constructors(obj: Object, is_updater = False) -> list:
    template = 'UpdaterTemplate' if is_updater else 'Template'

    if not obj.children or obj.is_union:
        return [
            f'using {obj.base_class}{template}::{obj.base_class}{template};'
        ]

    children = [f'{child.id}(*this, {index})' for index, child in enumerate(obj.children)]

    if obj.is_item:
        typename = obj.typename_updater if is_updater else obj.typename_contained
        const = '' if is_updater else 'const '
        return [
            '',
            f'{typename}() = default;',
            '',
            f'{typename}({const}ConfigDB::{obj.parent.base_class}& {obj.parent.id}, unsigned propIndex, uint16_t index):',
            [', '.join([
                f'{obj.classname}{template}({obj.parent.id}, propIndex, index)',
                *children
            ])],
            '{',
            '}',
        ]

    typename = obj.typename_updater if is_updater else obj.typename_contained
    parent_typename = obj.parent.typename_updater if is_updater else obj.parent.typename_contained
    headers = [
        '',
        f'{typename}() = default;',
        '',
        f'{typename}(ConfigDB::Object& parent, const ConfigDB::PropertyInfo& prop, uint16_t offset): ' + ', '.join([
            f'{obj.base_class}{template}(parent, prop, offset)',
            *children
        ]),
        '{',
        '}',
    ]

    if not obj.is_root:
        headers += [
            '',
            f'{typename}({parent_typename}& parent, unsigned propIndex): ' + ', '.join([
                f'{obj.base_class}{template}(parent, propIndex)',
                *children
            ]),
            '{',
            '}',
        ]

    return headers


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
            elif item is not None:
                f.write('\n')

    with open(filename, 'w', encoding='utf-8') as f:
        dump_output(comment + content, '')


def main():
    parser = argparse.ArgumentParser(description='ConfigDB specification utility')

    parser.add_argument('cfgfiles', nargs='+', help='Path to configuration file(s)')
    parser.add_argument('--outdir', help='Output directory')

    args = parser.parse_args()

    for f in args.cfgfiles:
        name = os.path.splitext(os.path.basename(f))[0]
        schemas[name] = load_schema(f)

    for name, schema in schemas.items():
        db = parse_schema(name, schema)

        lines = generate_database(db)

        filepath = os.path.join(args.outdir, f'{db.name}')
        os.makedirs(args.outdir, exist_ok=True)

        write_file(lines.header, f'{filepath}.h')
        write_file(lines.source, f'{filepath}.cpp')


if __name__ == '__main__':
    main()
