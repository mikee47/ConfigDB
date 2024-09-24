ConfigDB
========

Sming library providing strongly typed JSON configuration database support.

Applications requiring complex non-volatile configuration data typically store this in JSON files.
Although libraries such as ArduinoJson provide flexible and easy access to these, there are a few drawbacks:

- Once configuration exceeds a certain size it becomes unmanageable as a single JSON file
- Configuration data is typically managed on an ad-hoc basis using structures, and code is required to read/write these structures
- Code must deal with various common conditions such as default values, range checking, formatting, etc.
- Clients such as web applications or other external processors require further code to interpret configuration data

Design goals:

- Describe data **once** using a standard schema
- Generate API code from the schema which ensures consistent access, applies default values, checks value ranges, etc.
- Make use of available compile-time checks where possible (definitely for value types)
- Allow use of existing tools to enable easy access to data in other languages and on other platforms
- Allow partitioning of database into multiple *stores* to control use of RAM and enable more efficient updates for related data groups
- Reduce RAM usage as database size increases
- Maintain good read/write performance
- Pluggable system to support backing stores other than ArduinoJson. For example, binary formats
- Keep application interface independent of implementation detail

Usage:

- Database is described in a JSON schema.
- A python script (tools/dbgen.py) parses the schema and generates class structure for application use.
- Content of database can be streamed to/from web clients


JsonSchema
----------

The database structure is defined using a standard `JSON schema <https://json-schema.org>`.

An initial schema can be created from an existing sample JSON configuration file using a generation tool such as https://github.com/saasquatch/json-schema-inferrer. Go to the online demo and paste in your JSON configuration. The resulting schema can then be edited and further customised.

The schema can then be used to provide auto-generated Standardised web editors can also be generated using tools such as https://github.com/json-editor/json-editor. Go to the online demo and scroll down to **Schema**.

.. note::

    Sming uses JSON schema for:

        - Hardware configuration `Sming/Components/Storage/schema.json`
        - IFS build scripts `Sming/Components/IFS/tools/fsbuild/schema.json`
        - USB config `Sming/Libraries/USB/schema.json`

    It's probably fair to consider it a standard part of the framework so this should be documented somewhere centrally and referred to.

Configuration JSON can be validated against the **.cfgdb** schema files using **check-jsonschema**::

  pip install check-jsonschema
  check-jsonschema --schemafile basic-config.cfgdb sample-config.json


Schema rules
------------

See the :sample:`Basic_Config` sample schema. The test application contains further examples.

- Root object is always a :cpp:class:`ConfigDB::Database`
- A database is always rooted in a directory
- An optional **include** array annotation can be added to specify additional header files required for custom types used in the schema.
- Contains one or more stores. The root (un-named) object is the primary store, with the filename **_root.json**.
- Immediate children of the root may have a **store** value attached to place them into a new store.
  This can have any value, typically **true**.
- A custom type can be defined for Property accessors using the **ctype** annotation. This means that with an IP address property, for example, you can use :cpp:class:`IpAddress` instead of :cpp:class:`String` because it can be constructed from a String. The database still stores the value internally as a regular String.
- `Enumerated Properties`_ can be defined for any type; **ctype** is used here to optionally define a custom **enum class** type for these values.
- Re-useable definitions can be created using the `$ref <https://json-schema.org/understanding-json-schema/structuring#dollarref>`__ schema keyword.
  Such definitions must be contained within the **$defs** section of the schema.
- `Arrays`_ of simple types (including Strings) or objects are supported.
- `Unions`_ can contain any one of a number of user-defined object types, and can be used in object arrays.


Aliases
-------

Properties may have alternative names to support reading legacy datasets. For example::

  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
      "trans_fin_interval": {
        "type":"object",
        "alias": "transfin_interval",
        "properties":{
          "type":"integer"
        }
      }
    }
  }

Existing JSON data using the old *transfin_interval* name will be accepted during loading.
When changes are made the new (canonical) name of *trans_fin_interval* will be used.

If multiple aliases are required for a property, provide them as a list.


Floating-point numbers
~~~~~~~~~~~~~~~~~~~~~~

Items with **number** type are considered floating-point values.
They are not stored internally as *float* or *double* but instead use a base-10 representation.

This provides more flexibility in how these values are used and allows applications to work
with very large or small numbers without requiring any floating-point arithmetic.

See :cpp:class:`ConfigDB::number_t` and :cpp:class:`ConfigDB::Number` for details.
There is also :cpp:class:`ConfigDB::const_number_t` to ease support for format conversion
at compile time.


Enumerated properties
~~~~~~~~~~~~~~~~~~~~~

.. highlight: json

JsonSchema offers the `enum <https://json-schema.org/understanding-json-schema/reference/enum>`__ keyword to restrict values to a set of known values. For example::

  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
      "color": {
        "type": "string",
        "enum": [
          "red",
          "green",
          "blue"
        ]
      }
    }
  }

ConfigDB treats these as an *indexed map*, so *red* has the index 0, *green* is 1 and *blue* 2. Indices are of type *uint8_t*. The example has an intrinsic *minimum* of 0 and *maximum* of 2. As with other numeric properties, attempting to set values outside this range are clipped.

The default is 0 (the first string in the list). If a default value is given in the schema, it must match an item in the *enum* array.

The corresponding `setColor`, `getColor` methods set or retrieve the value as a number. Adding *"ctype": "Color"* to the property will generate an *enum class* definition instead. This is the preferred approach.

The *color* value itself will be stored as a *string* with one of the given values. The *integer* and *number* types are also supported, which can be useful for generating constant lookup tables.


Arrays
~~~~~~

ConfigDB uses the **array** schema keyword to implement both *simple* arrays (containing integers, numbers or Strings) and *object* arrays.

Simple arrays are accessed via the :cpp:class:`ConfigDB::Array` class. All elements must be of the same type. A **default** value may be specified which is applied automatically for uninitialised stores. The :cpp:func:`ConfigDB::Object::loadArrayDefaults` method may also be used during updates to load these default definitions.

The :cpp:class:`ConfigDB::ObjectArray` type can be used for arrays of objects or unions. Default values are not currently supported for these.


Unions
~~~~~~

These are defined using the  `oneOf <https://json-schema.org/understanding-json-schema/reference/combining#oneOf>`__ schema keyword.

The *test* application contains an example of this in the *test-config-union.cfgdb* schema. It is used in the *Updates* test module.

Like a regular C++ *union*, a :cpp:class:`ConfigDB::Union` object has one or more object types overlaid in the same storage space. The size of the object is therefore governed by the size of the largest type stored. A `uint8_t` property tag indicates which type is stored.

The code generator produces an **asXXX** method for each type of object which can be stored. The application is responsible for checking which type is present via :cpp:func:`ConfigDB::Union::getTag`; if the wrong method is called, a runtime assertion will be generated.

The corresponding Union Updater class has a :cpp:func:`ConfigDB::Union::setTag` method. This changes the stored object type and initialises it to default values. This is done even if the tag value doesn't change so can be used to 'reset' an object to defaults. The code generator produces a **toXXX** method which sets the tag and returns the appropriate object type.

Note that items in **$defs** can also be non-object property types. For these, a type is *not* defined but instead used as a base definition which can be modified. Take a general *Pin* definition, for example::

  "Pin": {
    "type": "integer",
    "minimum": 0,
    "maximum": 63
  }

And in the main schema, use it like this::

  "pin": {
    "$ref": "#/$defs/Pin",
    "default": 13
  }

The code generator expands this property::

  "pin": {
    "type": "integer",
    "minimum": 0,
    "maximum": 63,
    "default": 13
  }

This can make the schema more readable, save duplication and simplify modification.

Note that no special type is defined in generated code. If a `ctype` annotation is present then that type must be defined elsewhere in the application.


Store loading / saving
----------------------

By default, stores are saved as JSON files to the local filesystem.

The code generator creates a default :cpp:class:`ConfigDB::Database` class.
This can be overridden to customise loading/saving behaviour.

The :cpp:func:`ConfigDB::Database::getFormat` method is called to get the storage format for a given Store.
A :cpp:class:`ConfigDB::Format` implementation provides various methods for serializing and de-serializing database and object content.

Currently only **json** is implemented - see :cpp:class:`ConfigDB::Json::format`.
Each store is contained in a separate file.
The name of the store forms the JSONPath prefix for any contained objects and values.

The :sample:`BasicConfig` sample demonstrates using the stream classes to read and write data from a web client.

.. important::

  Any invalid data in a JSON update file will produce a debug warning, but will not cause processing to stop.
  This behaviour can be changed by implementing a custom :cpp:func:`ConfigDB::Database::handleFormatError` method.


JSON Update mechanism
---------------------

.. highlight: json

The default streaming update (writing) behaviour is to **overwrite** only those values received.
This allows selective updating of properties. For example::

  {
      "security": {
          "api_secured": "false"
      }
  }

This updates the **api_secured** value in the database, leaving everything else unchanged.

Arrays are handled slightly differently. To *overwrites* the array with new values::

  "x": [1, 2, 3, 4]

To *clear* the array::

  "x": []

**Indexed array operations**

Array selectors can be used which operate in the same way as python list operations.
So **x[i]** corresponds to a single element at index i, **x[i:j]** is a 'slice' starting at index i and ending with index (j-1). Negative numbers refer to offsets from the end of the array, so **-1** is the last element.

When selecting a single array element **x[5]**, the provided index *must* exist in the array or import will fail.
When updating a range, index values equal to or greater than the array length will be treated as an append operation.

The following example operations demonstrate what happens with an initial JSON array **x**::

  {
    "x": [1, 2, 3, 4]
  }

The *result* value shows the value for *x* after the update operation.
The same operations are supported for arrays of other types, including objects.

*Update single item*::

  {
    "x[0]" : 8,
    "result": [8, 2, 3, 4]
  },
  {
    "x[2]" : 8,
    "result": [1, 2, 8, 4]
  },
  {
    "x[-1]" : 8,
    "result": [1, 2, 3, 8]
  }

*Update multiple items*

Note that the assigned value *must* be an array or the import will fail::

  {
    "x[0:2]" : [8, 9],
    "result": [8, 9, 3, 4]
  },
  {
    "x[1:1]": [8, 9],
    "result": [1, 8, 9, 2, 3, 4]
  },
    "x[1:2]": [8, 9],
    "result": [1, 8, 9, 3, 4]
  },
  {
    "x[2:]": [8, 9],
    "result": [1, 2, 8, 9]
  }

*Insert item*::

  {
    "x[3:0]" : [8],
    "result": [1, 2, 3, 8, 4]
  },
  {
    "x[3:3]": [8],
    "result": [1, 2, 3, 8, 4]
  },
  {
    "x[-1:]" : [8, 9],
    "result": [1, 2, 3, 8, 9]
  }

*Append item*::

  {
    "x[]": [8, 9],
    "result": [1, 2, 3, 4, 8, 9]
  },
  {
    "x[]": 8,
    "result": [1, 2, 3, 4, 8]
  }

*Append multiple items*::

  {
    "x[]": [8, 9],
    "result": [1, 2, 3, 4, 8, 9]
  },
  {
    "x[10:]": [8, 9],
    "result": [1, 2, 3, 4, 8, 9]
  }


**Object array selection**

The **x[name=value]** syntax can be used to select *one* object from an array of objects. Here's the test data::

  {
    "x": [
      {
        "name": "object 1",
        "value": 1
      },
      {
        "name": "object 2",
        "value": 2
      }
    ]
  }

And the selector can be used like this::

  {
    "x[name=object 1]": { "value": 8 },
    "result": [
      {
        "name": "object 1",
        "value": 8
      },
      {
        "name": "object 2",
        "value": 2
      }
    ]
  }

or::

  {
    "x[value=2]": { "value": 8 },
    "x[value=1]": { "value": 1234 },
    "result": [
      {
        "name": "object 1",
        "value": 1234
      },
      {
        "name": "object 2",
        "value": 8
      }
    ]
  }

Limitations:

- Only the first matching object will be selected
- Only one object key can be matched

You can find more examples in the test application under *resource/array-test.json*.


C++ API code generation
-----------------------

Each *.cfgdb* file found in the project directory is compiled into a corresponding *.h* and *.cpp* file in *out/ConfigDB*.
This directory is added to the *#include* path.

For example:

- *basic-config.cfgdb* is compiled into *basic-config.h* and *basic-config.cpp*
- The applications will *#include <basic-config.h>*
- This file contains defines the **BasicConfig** class which contains all accessible objects and array items
- Each object defined in the schema, such as *network*, gets a corresponding *contained* class such as **ContainedNetwork**, and an *outer* class such as **Network**.
- Both of these classes provide *read-only* access to the data via `getXXX` methods.
- Outer classes contain a :cpp:class:`ConfigDB::StoreRef`, whereas contained classes do not (they obtain the store from their parent object).
- Application code can instantiate the *outer* class directly **BasicConfig::Network network(database);**
- Child objects within classes are defined as member variables, such as **network.mqtt**, which is a **ContainedMqtt** class instance.
- A third *updater* class type is also generated which adds *setXXX* methods for changing values.
- Only one *updater* per store can be open at a time. This ensures consistent data updates.


Updaters
--------

.. highlight: c++

Code can update database entries in several ways.

1.  Using updater created on read-only class::

      BasicConfig::Root::Security sec(database);
      if(auto update = sec.update()) {
        update.setApiSecured(true);
      }

    The `update` value is a `BasicConfig::Root::Security::Updater` instance.

    Any changes made via the *update* are immediately reflected in *sec* as they share the same Store instance.
    The *update()* method can be called multiple times when used in this way.
    Changes are committed automatically when the last updater loses scope.

2.  Directly instantiate updater class::

      BasicConfig::Root::Security::Updater update(database);
      if(update) {
        update.setApiSecured(true);
      }

    Only *one* updater instance is permitted.

3.  Asynchronous update::

      BasicConfig::Root::Security sec(database);
      bool completed = sec.update([](auto update) {
        update.setApiSecured(true);
      });

    If there are no other updates in progress then the update happens immediately and *completed* is *true*.
    Otherwise the update is queued and *false* is returned. The update will be executed when the store is released.

During an update, applications can optionally call :cpp:func:`Updater::commit` to save changes at any time.
Changes are only saved if the Store *dirty* flag is set.
Calling :cpp:func:`Updater::clearDirty` will prevent auto-commit, provided further changes are not made.


API Reference
-------------

.. doxygenclass:: ConfigDB::Database
   :members:

.. doxygenclass:: ConfigDB::Store
   :members:

.. doxygenclass:: ConfigDB::Object
   :members:

.. doxygenclass:: ConfigDB::Union
   :members:

.. doxygenclass:: ConfigDB::Array
   :members:

.. doxygenclass:: ConfigDB::ObjectArray
   :members:

.. doxygenclass:: ConfigDB::Format
   :members:

.. doxygenvariable:: ConfigDB::Json::format

.. doxygenclass:: ConfigDB::Number
   :members:

.. doxygenstruct:: ConfigDB::number_t
   :members:

.. doxygenstruct:: ConfigDB::const_number_t
