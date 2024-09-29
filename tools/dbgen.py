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
    return 'union' if 'oneOf' in fields else fields['type']


@dataclass
class Property:
    parent: Property | Database
    name: str
    ref: dict
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

    def __init__(self, parent: Property, key: str, fields: dict):
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
        return self.obj.data_size if self.obj else CPP_TYPESIZES[self.ctype]

    @property
    def id(self):
        return make_identifier(self.name) if self.name else 'root'

    @property
    def typename(self):
        return make_typename(self.name)

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
    def is_store(self) -> bool:
        return isinstance(self.parent, Database)

    @property
    def store_prop(self) -> Property:
        return self if self.is_store else self.parent.store_prop

    @property
    def store(self) -> Object:
        return self.store_prop.obj

    @property
    def store_index(self):
        store = self.store_prop
        db: Database = store.parent
        return db.object_properties.index(store)

    @property
    def is_item(self):
        return not self.is_store and self.parent.obj.is_array

    @property
    def is_item_member(self):
        return self.is_item or self.parent.is_item_member

    @property
    def path(self):
        return join_path(self.parent.path, self.name) if self.parent else self.name

    @property
    def database(self):
        return self.store_prop.parent

    @property
    def namespace(self):
        assert self.obj
        if self.obj.ref or (self.is_item and self.parent.obj.ref):
            return self.database.typename
        prop = self.parent
        ns = []
        while isinstance(prop, Property):
            if not isinstance(prop.obj, ObjectArray):
                ns.insert(0, prop.obj.typename_contained)
            # obj = obj.database if obj.ref else obj.parent
            prop = prop.parent
        return '::'.join(ns)


@dataclass
class Object:
    name: str
    ref: str | None
    alias: str | list[str] | None
    schema_id: str = None
    object_properties: list[Property] = field(default_factory=list)
    properties: list[Property] = field(default_factory=list)

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
        return self.object_properties or self.properties

    @property
    def classname(self):
        return type(self).__name__

    @property
    def base_class(self):
        return f'{self.classname}'

    @property
    def is_array(self):
        return False

    @property
    def is_union(self):
        return False


@dataclass
class Array(Object):
    default: list = None

    @property
    def is_array(self):
        return True

    @property
    def items(self):
        return self.properties[0]

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
    default: list = None

    @property
    def is_array(self):
        return True

    @property
    def items(self):
        return self.object_properties[0]

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
        return max(prop.data_size for prop in self.object_properties) + UNION_TAG_SIZE


@dataclass
class Database(Object):
    schema: dict = None
    objects: dict[Object] = field(default_factory=dict)
    strings: StringTable = StringTable()
    forward_decls: set[str] = None

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
    with open(filename, 'r', encoding='utf-8') as f:
        schema = json.load(f)
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
    databases[schema_id] = Database(schema_id, None, None, schema_id=schema_id, schema=schema)


def resolve_ref(parent: Object, fields: dict, db: Database) -> tuple[str, dict]:
    '''If property contains a reference, deal with it

    Where the definition is for an object, that definition must be parsed and added to the corresponding database.
    However, that database may not exist yet...
    Maybe what we should do is use a separate structure for that.
    We have the global `schemas` dictionary, so could we just add the object definition to that?
    '''
    ref = fields.get('$ref')
    if not ref:
        return None, fields
    print("RESOLVE", parent.schema_id, fields)
    resolved_fields = None
    if ref.startswith('#/'):
        ref = ref[2:]
        schema_id = parent.schema_id or db.schema_id
        print("INT", schema_id)
    else:
        schema_id, _, ref = ref.partition('/')
        print("EXT", schema_id)
    db = databases[schema_id]
    x = db.schema
    print("REF", ref, x)
    while '/' in ref:
        node, _, ref = ref.partition('/')
        print("X", node, ref)
        x = x[node]
    resolved_fields = x[ref]

    '''So if this resolves to an object then we should process it and return the object.
    For simple properties we return a dictionary.
    '''
    if resolved_fields is None:
        raise ValueError('Cannot resolve ' + ref)

    # Has object already been parsed?
    obj = resolved_fields.get('object')
    if obj:
        return obj

    # Is this a simple 

    resolved_fields['schema_id'] = schema_id

    return ref, resolved_fields


def resolve_prop_ref(parent: Object, fields: dict, db: Database) -> None:
    '''Resolve reference and update fields'''
    ref, resolved_fields = resolve_ref(parent, fields, db)
    if ref is None:
        return

    fields['ref'] = ref

    # Object definitions are used unchanged: property just references it
    if get_ptype(resolved_fields) in ['object', 'union']:
        fields.update(resolved_fields)
        return

    # For simple properties the definition is a template,
    # so copy over any non-existent values
    for key, value in resolved_fields.items():
        if key not in fields:
            fields[key] = value;


def parse_database(database: Database):
    '''Validate and parse schema into python objects'''
    database.include = database.schema.get('include', [])
    root = Property(database, '', {'type':'object'})
    root.obj = Object('', None, None)
    database.object_properties.append(root)

    def parse_properties(parent_prop: Property, properties: dict):
        parent = parent_prop.obj
        for key, fields in properties.items():
            resolve_prop_ref(parent, fields, database)
            prop = Property(parent_prop, key, fields)
            if prop.ctype != '-': # object or array
                parent.properties.append(prop)
                continue

            # Objects with 'store' annotation are managed by database, otherwise they live in root object
            if 'store' in fields:
                if parent_prop is not root:
                    raise ValueError(f'{key} cannot have "store" annotation, not a root object')
                parse_object(database, key, fields)
            else:
                parse_object(parent_prop, key, fields)

    def parse_object(parent_prop: Property | Database, key: str, fields: dict) -> Property:
        print("parse_object", parent_prop.name, key, fields)
        parent = parent_prop if isinstance(parent_prop, Database) else parent_prop.obj
        ref, fields = resolve_ref(parent, fields, database)
        prop_type = get_ptype(fields)

        def createObjectProperty(Class) -> Property:
            prop = Property(parent_prop, key, fields)
            parent.object_properties.append(prop)
            prop.obj = Class(key, ref, fields.get('alias'), schema_id = fields.get('schema_id'))
            return prop

        if prop_type == 'object':
            if 'default' in fields:
                raise ValueError('Object default not supported (use default on properties)')
            object_prop = createObjectProperty(Object)
            parent.object_properties.append(object_prop)
            obj = object_prop.obj
            database.objects[obj.typename] = obj
            parse_properties(object_prop, fields.get('properties', {}))
            return object_prop

        if prop_type == 'array':
            item_ref, items = resolve_ref(parent, fields['items'], database)
            items_type = get_ptype(items)
            if items_type in ['object', 'union']:
                if 'default' in fields:
                    raise ValueError('ObjectArray default not supported')
                array_prop = createObjectProperty(ObjectArray)
                arr = array_prop.obj
                database.objects[arr.typename] = arr
                items_prop = Property(array_prop, '', {'type':'object'})
                arr.object_properties.append(items_prop)
                items_prop.obj = database.objects.get(item_ref)
                if items_prop.obj and items_prop.obj.ref:
                    return array_prop
                items_prop.obj = Object(item_ref or f'{arr.typename}Item', item_ref, None, schema_id=fields.get('schema_id'))
                database.objects[items_prop.obj.typename] = items_prop.obj
                if items_type == 'union':
                    parse_object(items_prop, item_ref, items)
                else:
                    parse_properties(items_prop, items['properties'])
                return array_prop

            # Simple array
            array_prop = createObjectProperty(Array)
            parse_properties(array_prop, {'items': items})
            arr = array_prop.obj
            assert len(arr.properties) == 1
            arr.default = fields.get('default')
            return array_prop

        if prop_type == 'union':
            if 'default' in fields:
                raise ValueError('Union default not supported')
            union_prop = createObjectProperty(Union)
            union = union_prop.obj
            database.objects[union.typename] = union
            for opt in fields['oneOf']:
                print("REF FIND", opt)
                ref, opt = resolve_ref(union, opt, database)
                print("REF RESULT", type(ref), ref, opt)
                choice = database.objects.get(ref)
                if choice:
                    choice_prop = Property(union_prop, ref, {'type':'object'})
                    choice_prop.obj = choice
                else:
                    print("REF NOT FOUND")
                    choice_prop = parse_object(union_prop, ref, opt)
                    choice = choice_prop.obj
                    choice.ref = ref
                    database.objects[choice.typename] = choice
                union.object_properties.append(choice_prop)
            return union_prop

        raise ValueError('Bad type ' + prop_type)

    parse_properties(root, database.schema['properties'])


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''

    db.forward_decls = {
        'ContainedRoot',
        'RootUpdater'
    }
    external_defs = []
    for obj in db.objects.values():
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
                f'{len(db.properties)},',
                '{',
                *(make_static_initializer(
                    [
                    '.type = PropertyType::Object',
                    f'.name = {db.strings[prop.name]}',
                    '.offset = 0',
                    f'.variant = {{.object = &{prop.obj.typename_contained}::typeinfo}}'
                    ], ',') for prop in db.properties),
                '}'
            ],
            '};'
        ])

    for obj in reversed(db.objects.values()):
        if obj.ref:
            if obj.schema_id:
                pass
            else:
                lines.append(generate_object(db, obj))

    for prop in db.object_properties:
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
            f'class {obj.typename_outer}: public {template}',
            '{',
            'public:',
            [
                'using OuterObjectTemplate::OuterObjectTemplate;',
            ],
            *[generate_outer_class(obj, prop, store_offset) for prop in obj.object_properties if not object_prop.is_item],
            '};'
        ] if obj.object_properties and not obj.is_union else [
            f'using {obj.typename_outer} = {template};'
        ]
    for prop in db.object_properties:
        lines.header.append(generate_outer_class(db, prop, 0))

    lines.header += ['};']

    for obj in db.objects.values():
        if not obj.is_union:
            continue
        decl = f'String toString({db.typename}::{obj.typename_contained}::Tag tag)'
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
            if not prop.obj.is_union:
                offset += prop.data_size
        indent += 1
        for prop in obj.properties:
            add(offset, prop.id, prop.ctype)
            if not obj.is_union:
                offset += prop.data_size
        if obj.is_union:
            add(offset + obj.data_size - UNION_TAG_SIZE, 'tag', 'uint8_t')
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
            f'.variant = {{.object = &{prop.namespace}::{prop.obj.typename_contained}::typeinfo}}'
        ]]
        add_alias(prop.alias)
        if not prop.obj.is_union:
            offset += prop.data_size

    if obj.is_union:
        proplist += [[
            '.type = PropertyType::UInt8',
            f'.name = {db.strings['tag']}',
            f'.offset = {object_prop.data_size - UNION_TAG_SIZE}',
        ]]

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

    if object_prop.data_size == 0:
        return CodeLines()

    def get_ctype(prop):
        return 'ConfigDB::StringId' if prop.ptype == 'string' else prop.ctype

    def get_default(prop):
        if prop.ptype == 'string':
            return make_comment(prop.default) if prop.default else ''
        return prop.default_str

    obj = object_prop.obj

    children = [prop for prop in obj.object_properties if prop.data_size]

    if obj.is_union:
        body = [
            'union __attribute__((packed)) {',
            [f'{prop.obj.typename_struct} {prop.id}{"{}" if index == 0 else ""};' for index, prop in enumerate(children)],
            '};',
            f'{UNION_TAG_TYPE} tag{{0}};',
        ]
    else:
        body = [
            [f'{prop.obj.typename_struct} {prop.id}{{}};' for prop in children] if children else None,
            [f'{get_ctype(prop)} {prop.id}{{{get_default(prop)}}};' for prop in obj.properties]
        ]

    typename = f'{object_prop.namespace}::{obj.typename_contained}'
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

    if isinstance(obj, ObjectArray):
        item_lines = CodeLines() if obj.items.ref else generate_object(db, obj.items)
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
            *declare_templated_class(obj, [obj.items.obj.typename_updater], True),
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
            *(f'{prop.obj.typename_updater} {prop.id};' for prop in obj.object_properties)
        ],
        '};'
    ]


def generate_contained_constructors(object_prop: Property, is_updater = False) -> list:
    template = 'UpdaterTemplate' if is_updater else 'Template'

    obj = object_prop.obj

    if not obj.object_properties or obj.is_union:
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
    parent_typename = object_prop.parent.obj.typename_updater if is_updater else object_prop.parent.obj.typename_contained
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

    if True:# not obj.is_root:
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
        load_schema(f)

    for db in databases.values():
        parse_database(db)

    for db in databases.values():
        lines = generate_database(db)

        filepath = os.path.join(args.outdir, f'{db.name}')
        os.makedirs(args.outdir, exist_ok=True)

        write_file(lines.header, f'{filepath}.h')
        write_file(lines.source, f'{filepath}.cpp')


if __name__ == '__main__':
    main()
