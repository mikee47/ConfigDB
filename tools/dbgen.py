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
}

STRING_ID_SIZE = 2
ARRAY_ID_SIZE = 2
UNION_TAG_SIZE = 1

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
    'String': STRING_ID_SIZE,
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


def get_ptype(fields: dict):
    '''Identify a pseudo-type 'union' which makes script a bit clearer'''
    return 'union' if 'oneOf' in fields else fields['type']


@dataclass
class Property:
    name: str
    fields: dict
    ref: dict | None = None

    def validate(self):
        r = self.get_intrange()
        if not r:
            return
        if not (r.minimum <= (self.default or 0) <= r.maximum):
            raise ValueError(f'{self.name} bad default {self.default}: {r.minimum} <= value <= {r.maximum}')

    @property
    def ptype(self):
        return get_ptype(self.fields)

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
    def ctype_ret(self):
        '''Type to use for accessor return value'''
        return self.fields.get('ctype', self.ctype)

    @property
    def property_type(self):
        if self.ptype != 'integer':
            return self.ptype.capitalize()
        r = self.get_intrange()
        return f'Int{r.bits}' if r.is_signed else f'UInt{r.bits}'

    @property
    def propdata_id(self):
        return self.property_type.lower()

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


@dataclass
class Object:
    parent: 'Object'
    name: str
    properties: list[Property] = field(default_factory=list)
    children: list['Object'] = field(default_factory=list)
    ref: Object | None = None

    @property
    def store(self):
        return self if isinstance(self.parent, Database) else self.parent.store

    @property
    def typename(self):
        return self.ref or (make_typename(self.name) if self.name else 'Root')

    @property
    def typename_contained(self):
        return 'Contained' + self.typename

    @property
    def typename_updater(self):
        return f'{self.typename}Updater'

    @property
    def typename_struct(self):
        return self.typename_contained + '::Struct'

    @property
    def data_size(self):
        '''Size of the corresponding C++ storage'''
        return sum(prop.data_size for prop in self.properties)

    @property
    def namespace(self):
        if self.ref or (self.is_item and self.parent.ref):
            return self.store.parent.typename
        obj = self.parent
        ns = []
        while obj:
            if not isinstance(obj, ObjectArray):
                ns.insert(0, obj.typename_contained)
            obj = obj.parent
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
        return isinstance(self.parent, Database)

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


@dataclass
class Database(Object):
    definitions: dict = None
    object_defs: dict[Object] = field(default_factory=dict)

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


def resolve_ref(fields: dict, db: Database) -> tuple[str, dict]:
    '''If property contains a reference, deal with it'''
    ref = fields.get('$ref')
    if not ref:
        return None, fields
    if not db.definitions:
        raise ValueError('Schema missing definitions')
    prefix = '#/$defs/'
    resolved_fields = None
    if ref.startswith(prefix):
        ref = ref[len(prefix):]
        resolved_fields = db.definitions.get(ref)
    if not resolved_fields:
        raise ValueError('Cannot resolve ' + ref)
    return ref, resolved_fields


def resolve_prop_ref(prop: Property, db: Database) -> dict:
    '''If property contains a reference, deal with it'''
    prop.ref, fields = resolve_ref(prop.fields, db)
    if prop.ref is None:
        return prop.fields

    # Object definitions are used as-is
    if get_ptype(fields) in ['object', 'union']:
        prop.fields = fields
        return fields

    # For simple properties the definition is a template,
    # so copy over any non-existent values
    for key, value in fields.items():
        if key not in prop.fields:
            prop.fields[key] = value;
    return fields


def load_config(filename: str) -> Database:
    '''Load, validate and parse schema into python objects'''
    config = load_schema(filename)
    dbname = os.path.splitext(os.path.basename(filename))[0]
    database = Database(None, dbname, properties=config['properties'], definitions=config.get('$defs'))
    database.include = config.get('include', [])
    root = Object(database, '')
    database.children.append(root)

    def parse_properties(parent: Object, properties: dict):
        for key, value in properties.items():
            # Filter out support property types
            prop = Property(key, value)
            resolve_prop_ref(prop, database)
            if not prop.ctype:
                print(f'*** "{parent.path}": {prop.ptype} type not yet implemented.')
                continue
            prop.validate()
            if prop.ctype != '-': # object or array
                parent.properties.append(prop)
                continue

            # Objects with 'store' annoation are managed by database, otherwise they live in root object
            if 'store' in value:
                if parent is not root:
                    raise ValueError(f'{key} cannot have "store", not a root object')
                obj = database
            else:
                obj = parent

            child = parse_object(obj, key, value)
            if child:
                obj.children.append(child)

    def parse_object(parent: Object, key: str, fields: dict) -> Object:
        ref, fields = resolve_ref(fields, database)
        prop_type = get_ptype(fields)

        if prop_type == 'object':
            obj = Object(parent, key)
            if ref:
                obj.ref = ref
                database.object_defs[ref] = obj
            parse_properties(obj, fields['properties'])
            return obj

        if prop_type == 'array':
            item_ref, items = resolve_ref(fields['items'], database)
            items_type = get_ptype(items)
            if items_type in ['object', 'union']:
                arr = ObjectArray(parent, key)
                if ref:
                    arr.ref = ref
                    database.object_defs[ref] = arr
                arr.items = database.object_defs.get(item_ref)
                if not arr.items:
                    arr.items = Object(arr, item_ref or f'{arr.typename}Item')
                    if item_ref:
                        arr.items.ref = item_ref
                        database.object_defs[item_ref] = arr.items
                    if items_type == 'union':
                        parse_object(arr.items, item_ref, items)
                    else:
                        parse_properties(arr.items, items['properties'])
                return arr

            # Simple array
            arr = Array(parent, key)
            parse_properties(arr, {'items': items})
            assert len(arr.properties) == 1
            arr.items = arr.properties[0]
            arr.properties = []
            return arr

        if prop_type == 'union':
            union = Union(parent, key)
            if ref:
                union.ref = ref
                database.object_defs[ref] = union
            for opt in fields['oneOf']:
                ref, opt = resolve_ref(opt, database)
                choice = database.object_defs.get(ref)
                if not choice:
                    choice = parse_object(union, ref, opt)
                    choice.ref = ref
                    database.object_defs[ref] = choice
                union.children.append(choice)
            return union

        raise ValueError('Bad type ' + prop_type)

    parse_properties(root, database.properties)
    return database


def generate_database(db: Database) -> CodeLines:
    '''Generate content for entire database'''

    lines = CodeLines(
        [
            '#include <ConfigDB/Database.h>',
            *(f'#include <{file}>' for file in db.include),
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
                '',
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
                f'{strings[db.name]},',
                f'{len(db.children)},',
                '{',
                [f'&{store.typename}::typeinfo,' for store in db.children],
                '}'
            ],
            '};'
        ])

    for obj in reversed(db.object_defs.values()):
        lines.append(generate_object(obj))

    for store in db.children:
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
    def generate_outer_class(obj: Object) -> list:
        children = dict((child.typename, child) for child in obj.children).values()
        return [
            '',
            f'class {obj.typename}: public ConfigDB::OuterObjectTemplate<{obj.typename_contained}, {obj.typename_updater}, {obj.store.typename_contained}>',
            '{',
            'public:',
            [
                'using OuterObjectTemplate::OuterObjectTemplate;',
            ],
            *[generate_outer_class(child) for child in children if not obj.is_item],
            '};'
        ] if children and not obj.is_union else [
            f'using {obj.typename} = ConfigDB::OuterObjectTemplate<{obj.typename_contained}, {obj.typename_updater}, {obj.store.typename_contained}>;'
        ]
    for store in db.children:
        lines.header.append(generate_outer_class(store))

    lines.header += ['};']

    def generate_global_functions(obj: Object):
        if obj.is_union:
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
                    [f'case {index}: return {strings[child.name]};' for index, child in enumerate(obj.children)],
                    ['default: return nullptr;'],
                    '}'
                ],
                '}'
            ]
        for child in obj.children:
            generate_global_functions(child)
    for obj in db.children:
        generate_global_functions(obj)


    # Insert this at end once string table has been populated
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

    lines = CodeLines([], [])

    lines.header += [
        'static const ConfigDB::ObjectInfo typeinfo;'
    ]

    def getVariantInfo(prop: Property) -> list:
        if prop.ptype == 'string':
            fstr = strings[str(prop.default or '')]
            return [f'{{.defaultString = &{fstr}}}']
        r = prop.get_intrange()
        if r is None:
            return []
        if r.bits > 32:
            tag = 'int64' if r.is_signed else 'uint64'
            return [f'.{tag} = {{&{prop.name}_minimum, &{prop.name}_maximum}}']
        tag = 'int32' if r.is_signed else 'uint32'
        return [f'.{tag} = {{{r.minimum}, {r.maximum}}}']

    proplist = []
    offset = UNION_TAG_SIZE if obj.is_union else 0

    objinfo = [obj.items] if isinstance(obj, ObjectArray) else obj.children
    for child in objinfo:
        proplist += [[
            'PropertyType::Object',
            'fstr_empty' if obj.is_array else strings[child.name],
            offset,
            f'{{.object = &{child.namespace}::{child.typename_contained}::typeinfo}}'
        ]]
        if not obj.is_union:
            offset += child.data_size

    if obj.is_union:
        proplist += [[
            'PropertyType::UInt8',
            strings['tag'],
            0,
        ]]

    propinfo = [obj.items] if isinstance(obj, Array) else obj.properties
    for prop in propinfo:
        r = prop.get_intrange()
        if r and r.bits > 32:
            lines.source += [f'constexpr const {prop.ctype} PROGMEM {prop.name}_minimum = {r.minimum};']
            lines.source += [f'constexpr const {prop.ctype} PROGMEM {prop.name}_maximum = {r.maximum};']
        proplist += [[
            f'PropertyType::{prop.property_type}',
            'fstr_empty' if obj.is_array else strings[prop.name],
            offset,
            *getVariantInfo(prop)
        ]]
        offset += prop.data_size

    lines.source += [
        '',
        f'const ObjectInfo {obj.namespace}::{obj.typename_contained}::typeinfo PROGMEM',
        '{',
        *([str(e) + ','] for e in [
            'ObjectType::' + ('Store' if obj.is_root else obj.classname),
            'fstr_empty' if obj.is_item else strings[obj.name],
            'nullptr' if obj.is_array else '&defaultData',
            'sizeof(ArrayId)' if obj.is_array else 'sizeof(Struct)',
            len(objinfo),
            len(propinfo) + obj.is_union,
        ]),
        [
            '{',
            *(make_static_initializer(prop, ',') for prop in proplist),
            '}'
        ],
        '};'
    ]

    return lines


def generate_object_struct(obj: Object) -> CodeLines:
    '''Generate struct definition for this object'''

    def get_ctype(prop):
        return 'ConfigDB::StringId' if prop.ptype == 'string' else prop.ctype

    def get_default(prop):
        if prop.ptype == 'string':
            return make_comment(prop.default) if prop.default else ''
        return prop.default_str

    members = [
        *(f'{child.typename_struct} {child.id};' for child in obj.children),
        *(f'{get_ctype(prop)} {prop.id}{{{get_default(prop)}}};' for prop in obj.properties)
    ]

    if obj.is_union:
        body = [
            'uint8_t tag{0};' if obj.is_union else '',
            'union __attribute__((packed)) {',
            members,
            '};'
        ]
    else:
        body = members

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
                [f'{child.name},' for child in obj.children],
                '};',
                '',
                'Tag getTag() const',
                '{',
                ['return Tag(Union::getTag());'],
                '}'
            ],
            *((
                '',
                f'const {child.typename_contained} get{child.name}() const',
                '{',
                [
                    f'assert(getTag() == Tag::{child.name});',
                    f'return {child.typename_contained}(*this, {UNION_TAG_SIZE});'
                ],
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
        ctype = prop.fields.get('ctype')
        if ctype:
            return f'const {ctype}&'
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
                f'{child.typename_updater} get{child.name}()',
                '{',
                [
                    f'assert(getTag() == Tag::{child.name});',
                    f'return {child.typename_updater}(*this, {UNION_TAG_SIZE});'
                ],
                '}',
                ) for index, child in enumerate(obj.children))
        ]

    return [*((
        '',
        f'void set{prop.typename}({get_ctype(prop)} value)',
        '{',
        [f'setPropertyValue({index}, {get_value_expr(prop)});'],
        '}'
        ) for index, prop in enumerate(obj.properties))]


def generate_object(obj: Object) -> CodeLines:
    '''Generate code for Object implementation'''

    typeinfo = generate_typeinfo(obj)
    constructors = generate_contained_constructors(obj)
    updater = generate_updater(obj)
    updater_fwd = [
        '',
        f'class {obj.typename_updater};'
    ]

    if isinstance(obj, ObjectArray):
        item_lines = CodeLines([], []) if obj.items.ref else generate_object(obj.items)
        return CodeLines(
            [
                *updater_fwd,
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
                *updater_fwd,
                *declare_templated_class(obj, [obj.items.ctype]),
                typeinfo.header,
                constructors,
                '};',
                *updater,
            ],
            typeinfo.source)

    lines = CodeLines(
        [
            *updater_fwd,
            *declare_templated_class(obj),
            [f'using Updater = {obj.typename_updater};'],
            typeinfo.header,
        ],
        [])

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
        return [
            *declare_templated_class(obj, [obj.items.typename_updater], True),
            constructors,
            '};',
        ]

    if isinstance(obj, Array):
        return [
            *declare_templated_class(obj, [obj.items.ctype], True),
            constructors,
            '};',
        ]

    return [
        *declare_templated_class(obj, [], True),
        constructors,
        *generate_property_write_accessors(obj),
        None if obj.is_union else [
            '',
            (f'{child.typename_updater} {child.id};' for child in obj.children)
        ],
        '};'
    ]


def generate_contained_constructors(obj: Object, is_updater = False) -> list:
    template = 'UpdaterTemplate' if is_updater else 'Template'

    if not obj.children or obj.is_union:
        return [
            f'using {obj.base_class}{template}::{obj.base_class}{template};'
        ]

    if obj.is_item:
        typename = obj.typename_updater if is_updater else obj.typename_contained
        const = '' if is_updater else 'const '
        return [
            '',
            f'{typename}({const}ConfigDB::{obj.parent.base_class}& {obj.parent.id}, uint16_t dataRef):',
            [', '.join([
                f'{obj.classname}{template}({obj.parent.id}, dataRef)',
                *(f'{child.id}(*this, offsetof(Struct, {child.id}))' for child in obj.children)
            ])],
            '{',
            '}',
        ]

    typename = obj.typename_updater if is_updater else obj.typename_contained
    parent_typename = obj.parent.typename_updater if is_updater else obj.parent.typename_contained
    headers = []
    if not obj.is_item_member:
        headers = [
            '',
            f'{typename}(ConfigDB::Store& store): ' + ', '.join([
                f'{obj.base_class}{template}(store)',
                *(f'{child.id}(*this, offsetof(Struct, {child.id}))' for child in obj.children)
            ]),
            '{',
            '}',
        ]

    if not obj.is_root:
        headers += [
            '',
            f'{typename}({parent_typename}& parent, uint16_t dataRef): ' + ', '.join([
                f'{obj.base_class}{template}(parent, dataRef)',
                *(f'{child.id}(*this, offsetof(Struct, {child.id}))' for child in obj.children)
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
