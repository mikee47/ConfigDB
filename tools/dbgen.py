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

databases: dict[str, 'Database'] = {}

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
    if 'oneOf' in fields:
        return 'union'
    ptype = fields.get('type')
    if not ptype:
        raise ValueError(f'"type" is required')
    return ptype


@dataclass
class Property:
    parent: Property | Database
    name: str
    ptype: str
    ctype: str
    property_type: str
    ctype_override: str | None
    default: str | int | None
    alias: str | list[str] | None
    enum: list | None
    enum_type: str | None = None
    enum_ctype: str | None = None
    obj: Object | None = None
    is_store: bool = False

    def __init__(self, parent: Property, key: str, fields: dict):
        def error(msg: str):
            raise ValueError(f'"{parent.path}": {msg} for "{self.name}"')

        self.parent = parent
        self.name = key
        self.ptype = get_ptype(fields)
        self.ctype_override = fields.get('ctype')
        self.default = fields.get('default')
        self.ctype = CPP_TYPENAMES[self.ptype]
        self.property_type = self.ptype.capitalize()
        self.alias = fields.get('alias')
        self.enum = fields.get('enum')
        is_store = fields.get('store')
        if is_store is not None:
            self.is_store = True
            if not isinstance(is_store, bool):
                print(f'WARNING: bool expected for "{key}/properties/store", found "{is_store}"')

        minval = fields.get('minimum')
        self.validate_type(minval, 'minimum')
        maxval = fields.get('maximum')
        self.validate_type(maxval, 'maximum')

        if self.ptype == 'integer':
            int32 = IntRange(0, 0, True, 32)
            if minval is None:
                minval = int32.typemin
            if maxval is None:
                maxval = int32.typemax
            self.intrange = r = IntRange.deduce(minval, maxval)
            r.check(self.default or 0)
            self.ctype = r.ctype
            self.property_type = r.property_type
        elif self.ptype == 'number':
            if minval is None:
                minval = NUMBER_MIN
            if maxval is None:
                maxval = NUMBER_MAX
            self.numrange = r = Range(minval, maxval)
            r.check(self.default or 0)

        if not self.ctype:
            error(f'Invalid property type "{self.ptype}"')

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

        self.validate_type(self.default, 'default')

    def validate_type(self, value, attr_name: str) -> None:
        '''Verify that if value is given it is of the correct schema type'''
        if value is None:
            return
        if isinstance(value, list) and self.ptype != 'array':
            for x in value:
                self.validate_type(x, attr_name)
            return
        if self.enum:
            return
        types = {
            'array': list,
            'string': str,
            'integer': int,
            'boolean': bool,
            'number': (float, int),
        }
        if not isinstance(value, types[self.ptype]):
            raise ValueError(f'Attribute "{attr_name}" must be an {self.ptype}, found {type(value).__name__} ({value})')

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
        return self.obj.data_size if self.obj else CPP_TYPESIZES[self.ctype]

    @property
    def id(self):
        return make_identifier(self.name) if self.name else 'root'

    @property
    def typename(self):
        return make_typename(self.name)

    @property
    def typename_outer(self):
        assert self.obj
        return make_typename(self.name or 'Root')

    @property
    def default_str(self):
        default = self.default
        if self.ptype == 'string':
            return self.parent.database.strings[default]
        if self.ptype == 'boolean':
            return 'true' if default else 'false'
        if self.ptype == 'number':
            return f'const_number_t({self.default})' if self.default else '0'
        return str(default) if default else 0

    @property
    def is_root(self):
        '''Is this the root store?'''
        return self.is_store and not self.name

    @property
    def store_index(self):
        prop = self
        while not prop.is_store:
            prop = prop.parent
        store = prop
        i = 0
        for prop in self.database.object_properties:
            if prop is store:
                return i
            if prop.is_store:
                i += 1
        assert False

    @property
    def is_item(self):
        return self.parent.obj.is_array

    @property
    def is_item_member(self):
        return self.is_item or self.parent.is_item_member

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name

    @property
    def database(self) -> Database:
        prop = self
        while not isinstance(prop, Database):
            prop = prop.parent
        return prop

    @property
    def namespace(self):
        assert self.obj
        if self.obj.ref or (self.is_item and self.parent.obj.ref):
            return self.database.typename
        prop = self.parent
        ns = []
        while prop:
            if not prop.obj.is_array:
                ns.insert(0, prop.obj.typename_contained)
            prop = prop.database if prop.obj.ref else prop.parent
        return '::'.join(ns)


class ObjectProperty(Property):
    def __init__(self, parent: Property, name: str, fields: dict, obj: 'Object'):
        if 'type' not in fields:
            fields['type'] = 'object'
        super().__init__(parent, name, fields)
        self.obj = obj


@dataclass
class Object:
    parent: Object
    name: str
    ref: str | None
    schema_id: str
    object_properties: list[Property] = field(default_factory=list)
    properties: list[Property] = field(default_factory=list)

    @property
    def namespace(self):
        ns = []
        obj = self.parent
        while obj:
            if not obj.is_array:
                ns.insert(0, obj.typename_contained)
            obj = obj.parent
        return '::'.join(ns)

    @property
    def typename(self):
        return make_typename(self.name or 'Root')

    @property
    def typename_contained(self):
        return 'Contained' + self.typename

    @property
    def typename_updater(self):
        return f'{self.typename}Updater'

    @property
    def typename_struct(self):
        return self.typename_contained + '::Struct'

    def get_offset(self, obj: Object):
        '''Offset of a child object in the data structure'''
        offset = 0
        for c in self.object_properties:
            if c.obj is obj:
                return offset
            offset += c.data_size
        assert False, 'Not a child'

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return sum(obj.data_size for obj in self.object_properties) + sum(prop.data_size for prop in self.properties)

    @property
    def has_struct(self):
        return bool(self.object_properties or self.properties)

    @property
    def classname(self):
        return type(self).__name__

    @property
    def base_class(self):
        return self.classname

    @property
    def is_array(self):
        return False

    @property
    def is_object_array(self):
        return False

    @property
    def is_union(self):
        return False

    def __lt__(self, obj: Object):
        '''Sort lists by dependency'''
        return obj.depends_on(self)

    def depends_on(self, other: Object):
        dependencies = []
        def scan(obj: Object):
            for prop in obj.object_properties:
                if other is prop.obj:
                    return True
                if prop.obj in dependencies:
                    continue
                dependencies.append(prop.obj)
                scan(prop.obj)
            return False
        return scan(self)


@dataclass
class Array(Object):
    default: list = None

    @property
    def classname(self):
        return 'ObjectArray' if self.is_object_array else 'Array'

    @property
    def is_array(self):
        return True

    @property
    def is_object_array(self):
        return self.is_array and bool(self.object_properties)

    @property
    def items(self):
        return self.object_properties[0] if self.is_object_array else self.properties[0]

    @property
    def base_class(self):
        return 'StringArray' if self.items.ptype == 'string' else self.classname

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
    def max_object_size(self):
        return max(prop.data_size for prop in self.object_properties)

    @property
    def max_property_size(self):
        return max(prop.data_size for prop in self.properties)

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return self.max_object_size + self.max_property_size


@dataclass
class Database(Object):
    schema: dict = None
    object_defs: dict[Object] = field(default_factory=dict)
    external_objects: dict[Object] = field(default_factory=dict)
    strings: StringTable = StringTable()
    forward_decls: set[str] = None
    include: set[str] = None

    @property
    def namespace(self):
        return make_typename(self.schema_id)

    @property
    def obj(self):
        return self

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


def load_schema(filename: str) -> Database:
    '''Load JSON configuration schema and validate
    '''
    def parse_object_pairs(pairs):
        d = {}
        identifiers = set()
        for k, v in pairs:
            id = make_identifier(k)
            if not id:
                raise ValueError(f'Invalid key "{k}"')
            if id in identifiers:
                raise ValueError(f'Key "{k}" produces duplicate identifier "{id}"')
            identifiers.add(id)
            d[k] = v
        return d
    with open(filename, 'r', encoding='utf-8') as f:
        schema = json.load(f, object_pairs_hook=parse_object_pairs)
    try:
        from jsonschema import Draft7Validator
        v = Draft7Validator(Draft7Validator.META_SCHEMA)
        errors = list(v.iter_errors(schema))
        if errors:
            for e in errors:
                print(f'{e.message} @ {e.path}')
            sys.exit(3)
    except ImportError as err:
        print(f'\n** WARNING! {err}: Cannot validate "{filename}", please run `make python-requirements` **\n\n')
    schema_id = os.path.splitext(os.path.basename(filename))[0]
    databases[schema_id] = Database(None, schema_id, None, schema_id=schema_id, schema=schema)


def parse_properties(path: str, parent_prop: Property, properties: dict):
    for key, fields in properties.items():
        parse_property(f'{path}/{key}', parent_prop, key, fields)


def parse_property(path: str, parent_prop: Property, key: str, fields: dict) -> Property:
    '''If property contains a reference, deal with it

    References can point to anywhere in the current database (prefixed with '#') or in another one.

        #/$defs/Color
        test-config-union/$defs/Color
        test-config                         The entire schema

    References can also be chained, so we need to follow them:

        test-config/properties/color1 -> `#/$defs/Color`

    A reference is parsed into an object definition on first use.
    We can put that definition directly into the schema so it only needs to be parsed once.
    No need for separate `Database.objects`, we use `Database.schema` instead.
    '''
    try:
        database = parent_prop.database
        schema_id = parent_prop.obj.schema_id or database.schema_id

        def create_object_property(obj: Object) -> Property:
            # Objects with 'store' annoation are managed by database, otherwise they live in root object
            if 'store' in fields:
                if not parent_prop.is_root:
                    raise ValueError(f'{path} cannot have "store" annotation, not a root object')
                prop = ObjectProperty(database, key, fields, obj)
                prop.is_store = True
                database.object_properties.append(prop)
            else:
                prop = ObjectProperty(parent_prop, key, fields, obj)
                parent_prop.obj.object_properties.append(prop)
            return prop

        if object_ref := fields.get('$ref'):
            while object_ref:
                if object_ref.startswith('#/'):
                    ref = object_ref[2:]
                    object_ref = f'{schema_id}/{ref}'
                else:
                    schema_id, _, ref = object_ref.partition('/')
                path = f'{path} -> /{object_ref}'
                db = databases.get(schema_id)
                if not db:
                    raise ValueError(f'Database "{schema_id}" not found')
                parent = db
                ref_node = db.schema
                while ref:
                    if 'object' in ref_node:
                        parent = ref_node['object']
                    name, _, ref = ref.partition('/')
                    ref_node = ref_node.get(name)
                    if not ref_node:
                        raise ValueError(f'Missing reference "{name}" in "{object_ref}"')
                object_name = name
                if not key:
                    key = name

                # Has object already been parsed?
                if obj := ref_node.get('object'):
                    if obj.schema_id != parent_prop.obj.schema_id:
                        database.external_objects[object_ref] = obj
                    return create_object_property(obj)

                if next_ref := ref_node.get('$ref'):
                    object_ref = next_ref
                else:
                    break
        else:
            object_name = key
            parent = parent_prop.obj
            ref_node = None

        prop_type = get_ptype(ref_node or fields)

        if prop_type not in ['object', 'array', 'union']:
            if ref_node:
                # For simple properties the definition is a template, so copy over any non-existent values
                for k, v in ref_node.items():
                    if k not in fields:
                        fields[k] = v

            prop = Property(parent_prop, key, fields)
            parent_prop.obj.properties.append(prop)
            return prop

        if ref_node:
            fields = ref_node

        def create_object_and_property(Class) -> Property:
            obj = Class(database if 'store' in fields else db if object_ref else parent, object_name, object_ref, schema_id)
            if object_ref:
                db.object_defs[object_ref] = obj
            if obj.schema_id != parent_prop.obj.schema_id:
                database.external_objects[object_ref] = obj
            fields['object'] = obj
            return create_object_property(obj)

        if prop_type == 'object':
            if 'default' in fields:
                raise ValueError('Object default not supported (use default on properties)')
            object_prop = create_object_and_property(Object)
            parse_properties(f'{path}/properties', object_prop, fields.get('properties', {}))
            return object_prop

        if prop_type == 'array':
            items = fields.get('items')
            if not items:
                raise ValueError(f'Missing "items" property for array')
            array_prop = create_object_and_property(Array)
            items_prop = parse_property(f'{path}/items', array_prop, f'{array_prop.typename}Item', items)
            array_prop.obj.default = fields.get('default')
            if array_prop.obj.is_object_array:
                if 'default' in fields:
                    raise ValueError('ObjectArray default not supported')
            items_prop.validate_type(array_prop.obj.default, 'default[]')
            return array_prop

        if prop_type == 'union':
            if 'default' in fields:
                raise ValueError('Union default not supported')
            union_prop = create_object_and_property(Union)
            for i, opt in enumerate(fields['oneOf']):
                prop = parse_property(f'{path}/oneOf/{i}', union_prop, opt.get('title'), opt)
                if not prop.obj:
                    raise ValueError(f'Union "{union_prop.name}" option type must be *object*')
                if not prop.id or not prop.obj.typename:
                    raise ValueError(f'Union "{union_prop.name}" option requires title or $ref')
            prop = Property(union_prop, 'tag', {
                'type': 'integer',
                'minimum': 0,
                'maximum': len(union_prop.obj.object_properties) - 1
            })
            union_prop.obj.properties.append(prop)
            return union_prop

        raise ValueError('Bad type ' + prop_type)
    except ValueError as e:
        raise RuntimeError(path) from e


def parse_database(database: Database):
    '''Validate and parse schema into python objects'''
    database.include = database.schema.get('include', set())
    root_obj = Object(database, '', None, database.schema_id)
    database.schema['object'] = root_obj
    root = ObjectProperty(database, '', {}, root_obj)
    database.object_properties.append(root)
    root.is_store = True
    parse_properties(f'/{database.name}/properties', root, database.schema.get('properties', {}))

def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''

    db.forward_decls = {
        'ContainedRoot',
        'RootUpdater'
    }

    external_defs = []
    for obj in db.external_objects.values():
        db.include.add(f'{obj.schema_id}.h')
        ns = obj.namespace
        external_defs += [
            f'using {obj.typename_contained} = {ns}::{obj.typename_contained};',
            f'using {obj.typename_updater} = {ns}::{obj.typename_updater};',
        ]

    for obj in db.object_defs.values():
        db.forward_decls |= {obj.typename_contained, obj.typename_updater}

    lines = CodeLines(
        [
            '/*',
            generate_structure(db),
            '*/',
            '#pragma once',
            '',
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
                f'{len(db.object_properties)},',
                '{',
                *(make_static_initializer(
                    [
                    '.type = PropertyType::Object',
                    f'.name = {db.strings[prop.name]}',
                    '.offset = 0',
                    f'.variant = {{.object = &{prop.obj.typename_contained}::typeinfo}}'
                    ], ',') for prop in db.object_properties),
                '}'
            ],
            '};'
        ])

    for obj in sorted(db.object_defs.values()):
        prop = ObjectProperty(db, obj.name, {}, obj)
        lines.append(generate_object(db, prop))

    for prop in reversed(db.object_properties):
        if not prop.obj.ref:
            lines.append(generate_object(db, prop))

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
    def generate_outer_class(parent: Object, object_prop: Property, store_offset: int) -> list:
        obj = object_prop.obj
        template = f'ConfigDB::OuterObjectTemplate<{obj.typename_contained}, {obj.typename_updater}, {db.typename}, {object_prop.store_index}, {parent.typename_contained}, {parent.object_properties.index(object_prop)}, {store_offset}>'
        if not object_prop.is_store:
            store_offset += parent.get_offset(obj)
        return [
            '',
            f'class {object_prop.typename_outer}: public {template}',
            '{',
            'public:',
            [
                'using OuterObjectTemplate::OuterObjectTemplate;',
            ],
            *[generate_outer_class(obj, prop, store_offset) for prop in obj.object_properties if not object_prop.is_item],
            '};'
        ] if obj.object_properties and not obj.is_union else [
            f'using {object_prop.typename_outer} = {template};'
        ]
    for prop in db.object_properties:
        lines.header.append(generate_outer_class(db, prop, 0))

    lines.header += ['};']

    unions = {}
    def find_unions(object_prop: ObjectProperty | Database):
        for prop in object_prop.obj.object_properties:
            obj = prop.obj
            if obj.is_union:
                if obj.schema_id == db.schema_id:
                    unions[f'{prop.namespace}::{obj.typename_contained}'] = obj
            else:
                find_unions(prop)
    find_unions(db)
    if unions:
        lines.header += ['']
        for typename, obj in unions.items():
            decl = f'String toString({typename}::Tag tag)'
            lines.header += [
                f'{decl};'
            ]
            lines.source += [
                '',
                decl,
                '{',
                [
                    'switch(unsigned(tag)) {',
                    [f'case {index}: return {db.strings[prop.name]};' for index, prop in enumerate(obj.object_properties)],
                    ['default: return nullptr;'],
                    '}'
                ],
                '}'
            ]

    # Insert this at end once string table has been populated
    lines.source[:0] = [
        f'#include "{db.name}.h"',
        '',
        '// Not all defined strings may be referenced here',
        '#pragma GCC diagnostic ignored "-Wunused-variable"',
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
    def print_structure(object_prop: Property, indent: int, offset: int):
        def add(offset: int, id: str, typename: str):
            structure.append(f'{offset:4} {"".ljust(indent*3)} {id}: {typename}')
        obj = object_prop.obj
        add(offset, object_prop.id, obj.base_class)
        if obj.is_array:
            return
        for prop in obj.object_properties:
            print_structure(prop, indent+1, offset)
            if not obj.is_union:
                offset += prop.data_size
        if obj.is_union:
            offset += obj.max_object_size
        indent += 1
        for prop in obj.properties:
            add(offset, prop.id, prop.ctype)
            if not obj.is_union:
                offset += prop.data_size
    for prop in db.object_properties:
        print_structure(prop, 0, 0)
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


def generate_typeinfo(db: Database, object_prop: Property) -> CodeLines:
    '''Generate type information'''

    lines = CodeLines(
        ['static const ConfigDB::ObjectInfo typeinfo;']
    )

    obj = object_prop.obj

    def getVariantInfo(prop: Property) -> list:
        if prop.enum:
            values = prop.enum
            obj_type = f'{object_prop.namespace}::{obj.typename_contained}'
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

    for prop in obj.object_properties:
        proplist += [[
            '.type = PropertyType::Object',
            '.name = ' + ('fstr_empty' if obj.is_array else db.strings[prop.name]),
            f'.offset = {offset}',
            f'.variant = {{.object = &{prop.obj.namespace}::{prop.obj.typename_contained}::typeinfo}}'
        ]]
        add_alias(prop.alias)
        if not obj.is_union:
            offset += prop.data_size

    if obj.is_union:
        offset += obj.max_object_size

    for prop in obj.properties:
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
        f'const ObjectInfo {object_prop.namespace}::{obj.typename_contained}::typeinfo PROGMEM',
        '{',
        *([str(e) + ','] for e in [
            f'.type = ObjectType::{obj.classname}',
            f'.defaultData = {defaultData}',
            '.structSize = ' + ('sizeof(ArrayId)' if obj.is_array else 'sizeof(Struct)' if obj.has_struct else '0'),
            f'.objectCount = {len(obj.object_properties)}',
            f'.propertyCount = {len(obj.properties)}',
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


def generate_object_struct(object_prop: Property) -> CodeLines:
    '''Generate struct definition for this object'''

    def get_ctype(prop):
        return 'ConfigDB::StringId' if prop.ptype == 'string' else prop.ctype

    def get_default(prop):
        if prop.ptype == 'string':
            return make_comment(prop.default) if prop.default else ''
        return prop.default_str

    obj = object_prop.obj

    typename = f'{object_prop.namespace}::{obj.typename_contained}'
    return CodeLines([
        '',
        'struct __attribute__((packed)) Struct {',
        [
            'union __attribute__((packed)) {',
            [f'{prop.obj.typename_struct} {prop.id}{"{}" if index == 0 else ""};' for index, prop in enumerate(obj.object_properties)],
            '};',
        ] if obj.is_union else
        [
            [f'{prop.obj.typename_struct} {prop.id}{{}};' for prop in obj.object_properties],
        ],
        [f'{get_ctype(prop)} {prop.id}{{{get_default(prop)}}};' for prop in obj.properties],
        '};',
        '',
        'static const Struct defaultData;',
    ],
    [
        '',
        f'const {typename}::Struct PROGMEM {typename}::defaultData{{}};'
    ])


def generate_property_accessors(obj: Object) -> list:
    '''Generate typed get/set methods for each property'''

    if obj.is_union:
        return [
            '',
            [
                'enum class Tag {',
                [f'{prop.obj.typename},' for prop in obj.object_properties],
                '};',
                '',
                'Tag getTag() const',
                '{',
                ['return Tag(Union::getTag());'],
                '}'
            ],
            *((
                '',
                f'const {prop.obj.typename_contained} as{prop.obj.typename}() const',
                '{',
                [f'return Union::as<{prop.obj.typename_contained}>({index});'],
                '}',
                ) for index, prop in enumerate(obj.object_properties))
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
                f'{prop.obj.typename_updater} as{prop.obj.typename}()',
                '{',
                [f'return Union::as<{prop.obj.typename_updater}>({index});'],
                '}',
                '',
                f'{prop.obj.typename_updater} to{prop.obj.typename}()',
                '{',
                [f'return Union::to<{prop.obj.typename_updater}>({index});'],
                '}',
                ) for index, prop in enumerate(obj.object_properties))
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


def generate_object(db: Database, object_prop: Property) -> CodeLines:
    '''Generate code for Object implementation'''

    obj = object_prop.obj

    typeinfo = generate_typeinfo(db, object_prop)
    constructors = generate_contained_constructors(object_prop)
    updater = generate_updater(object_prop)
    forward_decls = [] if obj.typename_updater in db.forward_decls else [
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

    if obj.is_object_array:
        item_lines = CodeLines() if obj.items.obj.ref else generate_object(db, obj.items)
        return CodeLines(
            [
                *forward_decls,
                *item_lines.header,
                *declare_templated_class(obj, [obj.items.obj.typename_contained]),
                typeinfo.header,
                constructors,
                '};',
                *updater,
            ],
            item_lines.source + typeinfo.source)

    if obj.is_array:
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
    for prop in obj.object_properties:
        if not prop.obj.ref:
            lines.append(generate_object(db, prop))

    lines.append(generate_object_struct(object_prop))
    lines.header += [
        constructors,
        *generate_property_accessors(obj)
    ]
    lines.source += typeinfo.source

    # Contained children member variables
    if obj.object_properties and not obj.is_union:
        lines.header += [
            '',
            [f'const {prop.obj.typename_contained} {prop.id};' for prop in obj.object_properties],
        ]

    lines.header += ['};']
    lines.header += updater

    return lines


def generate_updater(object_prop: Property) -> list:
    '''Generate code for Object Updater implementation'''

    constructors = generate_contained_constructors(object_prop, True)

    obj = object_prop.obj

    if obj.is_object_array:
        union_array_methods = [
            [
                '',
                f'{item.obj.typename_updater} addItem{item.typename}()',
                '{',
                [f'return ObjectArray::addItem<ConfigDB::Union>().to<{item.obj.typename_updater}>({tag});'],
                '}',
                '',
                f'{item.obj.typename_updater} insertItem{item.typename}(unsigned index)',
                '{',
                [f'return ObjectArray::insertItem<ConfigDB::Union>(index).to<{item.obj.typename_updater}>({tag});'],
                '}',
            ] for tag, item in enumerate(obj.items.obj.object_properties)
        ] if isinstance(obj.items.obj, Union) else []
        return [
            *declare_templated_class(obj, [obj.items.obj.typename_updater], True),
            constructors,
            *union_array_methods,
            '};',
        ]

    if obj.is_array:
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
            *(f'{prop.obj.typename_updater} {prop.id};' for prop in obj.object_properties)
        ],
        '};'
    ]


def generate_contained_constructors(object_prop: Property, is_updater = False) -> list:
    template = 'UpdaterTemplate' if is_updater else 'Template'

    obj = object_prop.obj

    if not obj.object_properties or obj.is_union or obj.is_array:
        return [
            f'using {obj.base_class}{template}::{obj.base_class}{template};'
        ]

    children = [f'{prop.id}(*this, {index})' for index, prop in enumerate(obj.object_properties)]

    if object_prop.is_item:
        typename = obj.typename_updater if is_updater else obj.typename_contained
        const = '' if is_updater else 'const '
        return [
            '',
            f'{typename}() = default;',
            '',
            f'{typename}({const}ConfigDB::{object_prop.parent.obj.base_class}& {object_prop.parent.id}, unsigned propIndex, uint16_t index):',
            [', '.join([
                f'{obj.classname}{template}({object_prop.parent.id}, propIndex, index)',
                *children
            ])],
            '{',
            '}',
        ]

    typename = obj.typename_updater if is_updater else obj.typename_contained
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

    if not object_prop.is_store:
        headers += [
            '',
            f'{typename}(ConfigDB::Object& parent, unsigned propIndex): ' + ', '.join([
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
    parser.add_argument('--outdir', required=True, help='Output directory')

    args = parser.parse_args()

    for f in args.cfgfiles:
        print(f'Loading "{f}"')
        load_schema(f)

    for db in databases.values():
        print(f'Parsing "{db.name}"')
        parse_database(db)

    for db in databases.values():
        lines = generate_database(db)

        filepath = os.path.join(args.outdir, f'{db.name}')
        os.makedirs(args.outdir, exist_ok=True)

        write_file(lines.header, f'{filepath}.h')
        write_file(lines.source, f'{filepath}.cpp')


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        if e.__cause__:
            print('ERROR:', e.__cause__, 'from', ', '.join(e.args))
        else:
            print(repr(e))
        sys.exit(1)
