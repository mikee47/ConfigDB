'''Script to generate array selector test cases
'''
from __future__ import annotations
import sys
import os
import re
import json
from dataclasses import dataclass

def get_default_vars() -> dict():
    '''Return test config value dictionary which can be modified without side-effects'''
    return {
        'int_array': [1, 2, 3, 4],
        'object_array': [
            {"intval": 1, "stringval": "a"},
            {"intval": 2, "stringval": "b"},
            {"intval": 3, "stringval": "c"},
            {"intval": 4, "stringval": "d"},
        ]
    }

def json_dumps(value):
    '''Compact JSON text'''
    return json.dumps(value, separators=(',', ':'))

# Default JSON text
OBJECT_ARRAY = json_dumps(get_default_vars()['object_array'])

'''Cases grouped by array type, then category containing list of test cases.
A test case is a python expression equivalent to the JSON operation we want.
In some cases this is invalid python so a tuple is given instead with the expected result.
Note that we deal with `x[] = ...` case in code since that *can* be evaluated with tweaking.
'''
TEST_CASES = {
    'int_array': {
        'Overwrite array': [
            'x = [5, 6, 7, 8]',
            'x[0:] = [8, 9]',
        ],
        'Clear array': [
            'x = []',
        ],
        'Update single item': [
            'x[0] = 8',
            'x[2] = 8',
            'x[-1] = 8',
            'x[4] = 8',
        ],
        'Update multiple items': [
            'x[0:2] = [8, 9]',
            'x[1:1] = [8, 9]',
            'x[1:2] = [8, 9]',
            'x[2:] = [8, 9]',
            'x[0:1] = 8',
            'x[pin] = 8',
        ],
        'Insert item': [
            'x[3:0] = [8]',
            'x[3:3] = [8]',
            'x[-1:] = [8, 9]',
        ],
        'Append items': [
            'x[] = [8, 9]',
            'x[10:] = [8, 9]',
            'x[10:] = 8',
            ('x[] = 8', '[1,2,3,4,8]'),
            'x[]] = 8',
        ]
    },
    'object_array': {
        'Overwrite array': [
            'x = [{"intval":5,"stringval":"e"},{"intval":6,"stringval":"f"},{"intval":7,"stringval":"g"},{"intval":8,"stringval":"h"}]',
            'x[0:] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
        ],
        'Clear array': [
            'x = []',
        ],
        'Update single item': [
            ('x[0] = {}', OBJECT_ARRAY),
            'x[0] = {"intval":8,"stringval":"baboo"}',
            'x[2] = {"intval":8,"stringval":"baboo"}',
            'x[-1] = {"intval":8,"stringval":"baboo"}',
            'x[4] = {"intval":8,"stringval":"baboo"}',
            'x[stringval=c] = {"intval":8,"stringval":"baboo"}',
            ('x[intval=1] = {}', OBJECT_ARRAY),
            ('x[intval=0] = {}', '"Bad selector"')
        ],
        'Update multiple items': [
            'x[0:2] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            'x[1:1] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            'x[1:2] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            'x[2:] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            ('x[0:1] = {}', '"Require array value"'),
            'x[pin] = {}',
        ],
        'Insert item': [
            'x[3:0] = [{"intval":8,"stringval":null}]',
            'x[3:3] = [{"intval":8,"stringval":null}]',
            'x[-1:] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
        ],
        'Append items': [
            'x[] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            'x[10:] = [{"intval":8,"stringval":null},{"intval":9,"stringval":null}]',
            ('x[10:] = {}', '"Require array value"')
        ]
    }
}


@dataclass
class StringPos:
    offset: int
    length: int

    def __str__(self):
        return f'{{{self.offset}, {self.length}}}'


@dataclass
class ArrayTestCase:
    expr: StringPos
    result: StringPos

    def __str__(self):
        return f'{{{self.expr}, {self.result}}}'


def parse_expression(array_name: str, expr: str | tuple[str, str]) -> tuple[str, str]:
    '''Parse a test expression and return the JSON text for (expr, result)'''
    if isinstance(expr, tuple):
        expr, result = expr
    else:
        result = None
    key, _, value = expr.partition(' = ')
    value = value.replace(' ','')
    key = key.replace('x', array_name)
    expr = f'{{"{key}": {value}}}'
    if result:
        return expr, result
    vars = get_default_vars()
    vars['null'] = None
    try:
        # Convert `x[] = ...` into valid python
        tmp_key = key.replace('[]', '[100:0]')
        if '=' in tmp_key:
            # Require valid python for `array[name=value]` expression
            m = re.match(r'^(.*)?\[(.*)?=(.*)?]', tmp_key)
            name, key_name, key_value = m.group(1), m.group(2), m.group(3)
            i = next(i for i, x in enumerate(vars[name]) if x[key_name] == key_value)
            # print(f'{name}; {key_name}; {key_value}; {i}; {vars[name]}; {value}')
            tmp_expr = f'{name}[{i}] = {value}'
        else:
            tmp_expr = f'{tmp_key} = {value}'
        exec(tmp_expr, None, vars)
        return expr, json_dumps(vars[array_name])
    except Exception as e:
        return expr, f'"{e}"'


def main():
    json_data: bytearray = b'{\n'  # JSON string data
    array_tests: dict[str, list[ArrayTestCase]] = {} # array_name, list[ArrayTestCase]

    for array_name, cases in TEST_CASES.items():
        if array_tests:
            json_data += b',\n'
        json_data += f'\t"{array_name}": {{\n'.encode()
        array_tests[array_name] = test_cases = []
        # print(f'static constexpr const ArrayTestCase {array_name}_test_cases[] PROGMEM {{')
        for case_index, (title, expressions) in enumerate(cases.items()):
            if case_index:
                json_data += b',\n'
            json_data += f'\t\t"{title}": [\n'.encode()
            for expr_index, expr in enumerate(expressions):
                expr, result = parse_expression(array_name, expr)
                expr = expr.encode()
                result = result.encode()
                if expr_index:
                    json_data += b',\n'
                json_data += b'\t\t\t{\n\t\t\t\t"expr": '
                expr_pos = StringPos(len(json_data), len(expr))
                json_data += expr + b',\n\t\t\t\t"result": '
                result_pos = StringPos(len(json_data), len(result))
                json_data += result + b'\n\t\t\t}'
                test_cases.append(ArrayTestCase(expr_pos, result_pos))
            json_data += b'\n\t\t]'
        json_data += b'\n\t}'
    json_data += b'\n}'

    if len(sys.argv) > 1:
        path = sys.argv[1]
        hdr_file = os.path.join(path, 'include/array-test.h')
        json_file = os.path.join(path, 'resource/array-test.json')
        print(f'Creating {json_file} and {hdr_file}')
        with open(json_file, 'wb') as f:
            f.write(json_data)
        with open(hdr_file, 'w') as f:
            f.write('''\
#pragma once

// clang-format off

#include <FlashString/Array.hpp>

''')
            for array_name, cases in array_tests.items():
                f.write(f'DEFINE_FSTR_ARRAY({array_name}_test_cases, ArrayTestCase,\n')
                f.write(',\n'.join(f'\t{case}' for case in cases) + ' )\n')
    else:
        print(json_data.decode())

if __name__ == '__main__':
    main()
