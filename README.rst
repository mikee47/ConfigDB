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
- Pluggable system to support backing stores other than JSON, such as binary formats
- Keep application interface independent of implementation detail

Usage:

- Database is described in a JSON schema.
- A python script (tools/dbgen.py) parses the schema and generates class structure for application use.
- Content of database can be streamed to/from web clients


JsonSchema
----------

The database structure is defined using a standard `JSON schema <https://json-schema.org>`__. A good introduction is to take the `Tour <https://tour.json-schema.org/>`__.

An initial schema can be created from an existing sample JSON configuration file using a generation tool such as https://github.com/saasquatch/json-schema-inferrer. Go to the online demo and paste in your JSON configuration. The resulting schema can then be edited and further customised.

Standardised web editors can also be generated using tools such as https://github.com/json-editor/json-editor. Go to the online demo and scroll down to **Schema**.

.. note::

    Sming uses JSON schema for:

        - Hardware configuration :source:`Sming/Components/Storage/schema.json`
        - IFS build scripts :source:`Sming/Components/IFS/tools/fsbuild/schema.json`
        - USB config :source:`Sming/Libraries/USB/schema.json`

    It's probably fair to consider it a standard part of the framework.

Configuration JSON is validated against the **.cfgdb** schema files as a pre-build step.
This can also be done separately using a tool such as  **check-jsonschema**::

  pip install check-jsonschema
  check-jsonschema --schemafile basic-config.cfgdb sample-config.json

Separate documentation can be generated from JSON Schema using various tools such as `JSON Schema for Humans <https://coveooss.github.io/json-schema-for-humans/>`__.
For example::

  python -m pip install json-schema-for-humans
  generate-schema-doc basic-config.cfgdb basic-config.md --config template_name=md


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
  Such definitions should be contained within the **$defs** section of the schema.
- `Arrays`_ of simple types (including Strings) or objects are supported.
- `Unions`_ can contain any one of a number of user-defined object types, and can be used in object arrays.
- Null values are not supported. If encountered in existing datasets then ConfigDB uses the default value for the property.
- Multiple property types, such as *"type": ["boolean", "null"]* are not supported. A type must be defined for all properties and must be a string value. This also applies to array items.

.. highlight: json

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


Integers
~~~~~~~~

Items with **integer** type default to a regular signed 32-bit :cpp:type:`int32_t`. This can be changed by setting a *minimum* and or *maximum* attribute. The database will then use the smallest C++ type appropriate for that range. If the range excludes negative values then an unsigned value will be used.

The maximum supported integer range is for a signed 64-bit integer.

Note that when setting values via *setXXX* methods, streaming updates or inside arrays, integers will be **clipped** to the defined range, never truncated. For example::

  "properties": {
    "Pin": {
      "type": "integer",
      "minimum": 0,
      "maximum": 63
    }
  }

This creates a *pin* property of type :cpp:type:`uint8_t`. The updater will have a *setPin(int64_t)* method, which if called with values outside of the range 0-63 will be clamped.


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


Ranges
~~~~~~

All *integer* and *number* properties have a :cpp:class:`TRange` definition available should applications require it.
Typically this is defined inside the containing object, such as 
**BasicConfig::Color::Brightness::redRange** for a property named **red**.
For simple arrays, it is for example **BasicConfig::General::Numbers::itemRange**.
For enumerated types, it is defined within the associated type, for example **TestConfigEnum::ColorType::range**. Within arrays of such types, this can also be found in the array itself **TestConfigEnum::Root::Colors::itemType.range**.


Arrays
~~~~~~

ConfigDB uses the **array** schema keyword to implement both *simple* arrays (containing integers, numbers or Strings) and *object* arrays.

Simple arrays are accessed via the :cpp:class:`ConfigDB::Array` class. All elements must be of the same type. A **default** value may be specified which is applied automatically for uninitialised stores. The :cpp:func:`ConfigDB::Object::loadArrayDefaults` method may also be used during updates to load these default definitions.

The :cpp:class:`ConfigDB::ObjectArray` type can be used for arrays of objects or unions. Default values are not currently supported for these.

.. important::

  Be very careful when removing items from an array (via :cpp:func:`Array::removeItem`).
  Don't hold Object or Property references to items in the array during the operation as they may become invalid if the array ordering changes.
  Do not rely on the item index for removing multiple items since indices change when removeItem is called.

  Do not rely on the item index for removal, but instead use the content of the object or item as criteria. For example:

  .. code-block:: c++

    for(unsigned index=0; index < array.getItemCount();) {
      auto item = array[index];
      if (want_to_delete(item)) {
        // This causes subsquent items to shift down one, so leave index where it is
        array.removeItem(index);
      } else {
        ++index;
      }
    }


Unions
~~~~~~

These are defined using the  `oneOf <https://json-schema.org/understanding-json-schema/reference/combining#oneOf>`__ schema keyword, which defines an array of option definitions.

Each definition can be defined using **$ref**. The name of the option will be taken from that definition, and can be overridden by adding a **title** keyword. Option definitions can also be given directly, in which case **title** is required.

The *test* application contains an example of this in the *test-config-union.cfgdb* schema. It is used in the *Updates* test module.

Like a regular C++ *union*, a :cpp:class:`ConfigDB::Union` object has one or more object types overlaid in the same storage space. The size of the object is therefore governed by the size of the largest type stored. A `uint8_t` property tag indicates which type is stored.

The code generator produces an **asXXX** method for each type of object which can be stored. The application is responsible for checking which type is present via :cpp:func:`ConfigDB::Union::getTag`; if the wrong method is called, a runtime assertion will be generated.

The corresponding Union Updater class has a :cpp:func:`ConfigDB::Union::setTag` method. This changes the stored object type and initialises it to default values. This is done even if the tag value doesn't change so can be used to 'reset' an object to defaults. The code generator produces a **toXXX** method which sets the tag and returns the appropriate object type.


Re-using objects
~~~~~~~~~~~~~~~~

JSON Schema describes ways to `structure complex schemas <https://json-schema.org/understanding-json-schema/structuring>`__.

Re-useable (shared) definitions are, by convention, placed under **$defs**.
These are referenced using the **$ref** keyword with JSON pointer syntax.

For example::

  "$ref": "#/$defs/MyObject"

Definitions from other schema may be used::

  "$ref": "other-schema/$defs/MyObject"

The *dbgen.py* code generator is passed the names of *all* schema found in the current Sming project, which are loaded and parsed as a set using the base name of the *.cfgdb* schema (without file extension) as its identifier.

.. note::
  
    The full URI resolution described by JSON Schema is not currently implemented.
    This would require **$id** annotations in all schema.

When using shared objects only the name of the related property can be changed.
For example::

  "font-color": {
    "foreground": {
      "$ref": "#/$defs/Color"
    },
    "background": {
      "$ref": "#/$defs/Color"
    }
  }

This generates a C++ property *fontColor* using a *FontColor* object definition which itself contains two properties: *foreground* and *background*. The object definition for both is *Color*.


Simple property definitions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

References to simple (non-object) property types are handled differently. A type is *not* defined but instead used as a base definition which can be modified. For example, we can provide a general *Pin* definition::

  "$defs": {
    "Pin": {
      "type": "integer",
      "minimum": 0,
      "maximum": 63
    }
  }

And use it like this::

  "properties": {
    "input-pin": {
      "$ref": "#/$defs/Pin",
      "default": 13
    },
    "output-pin": {
      "$ref": "#/$defs/Pin",
      "default": 4
    }
  }

This is identical to the following::

  "properties": {
    "input-pin": {
      "type": "integer",
      "minimum": 0,
      "maximum": 63,
      "default": 13
    }
    "output-pin": {
      "type": "integer",
      "minimum": 0,
      "maximum": 63,
      "default": 4
    }
  }

This approach can make the schema more readable, reduce duplication and simplify maintainance.

This example generates a `uint8_t` property value. A different type may be specified for property accessors using the `ctype` annotation.


Calculated Values
-----------------

Within a ConfigDB schema, if an attribute name is prefixed with @ then the attribute value will be evaluated and the result used as the actual value.
The expression must be given as a string, with variable names corresponding to environment variables.
See :doc:`/_inc/Tools/Python/evaluator/README` for details.

The ``.cfgdb`` schema are pre-processed on every build and the source files regenerated automatically if there is a change.
The pre-processed schema can be found in ``out/ConfigDB/schema/``, together with a *summary.txt* file.

An example is included in the test application:

.. code-block:: json

  "simple-string": {
    "type": "string",
    "@default": "SIMPLE_STRING"
  }

The pre-processed schema will contain a ``default`` attribute with the contents of the environment variable ``SIMPLE_STRING``.
An error will be given if named variable is not present in the environment.

To test this, build and run as follows:

.. code-block:: bash

  SIMPLE_STRING="donkey2" make -j run

The test application now fails as the schema default value has changed - "donkey" is expected.

.. note::

  Using both "@default" and "default" (for example) results in an error: only one form may be used.


Integer expressions
~~~~~~~~~~~~~~~~~~~

JSON does not support extended number formats, such as `0x12`, so this mechanism enables calculated values for numeric properties by adding the `@` prefix and using a string value:

.. code-block:: json

  "simple-int": {
    "type": "integer",
    "@default": "8 + 27",
    "minimum": 0,
    "@maximum": "0xffff"
  }


Array values
~~~~~~~~~~~~

If a calculated attribute value is an array, then each element is evaluated separately.
Arrays may contain a mixture of types, but only string values will be evalulated: others will be passed through unchanged.


.. code-block:: json

  "simple-array": {
    "type": "array",
    "items": {
      "type": "integer"
    },
    "@default": [
      "0x12",
      5,
      "0x37 * 2",
      100
    ]
  }


Dictionary values
~~~~~~~~~~~~~~~~~

To conditionally select from one of a number of options, provide a dictionary as the calculated attribute value.
The first key which evaluates as *True* is matched, and the corresponding value becomes the value for the property.
If none of the entries matches, an error is raised.

In this example, the default contents of the *pin-list* array is determined by the targetted SOC.
The final *"True": []* ensures a value is provided if nothing else is matched.

.. code-block:: json

  "pin-list": {
    "type": "array",
    "items": {
      "type": "integer",
      "minimum": 0,
      "maximum": 255
    },
    "@default": {
      "SMING_SOC == 'esp8266'": [ 1, 2, 3, 4 ],
      "SMING_SOC == 'esp32c3'": [ 5, 6, 7, 8 ],
      "SMING_SOC == 'esp32s2'": [ 9, 10, 11, 12 ],
      "SMING_SOC in ['rp2040', 'rp2350']": [ 13, 14, 15, 16 ],
      "True": []
    }
  }


Store loading / saving
----------------------

By default, stores are saved as JSON files to the local filesystem.

The code generator creates a default :cpp:class:`ConfigDB::Database` class.
This can be overridden to customise loading/saving behaviour.

The :cpp:func:`ConfigDB::Database::getFormat` method is called to get the storage format for a given Store.
A :cpp:class:`ConfigDB::Format` implementation provides various methods for serializing and de-serializing database and object content.

Currently only **json** is implemented - see :cpp:member:`ConfigDB::Json::format`.
Each store is contained in a separate file.
The name of the store forms the JSONPath prefix for any contained objects and values.

The :sample:`Basic_Config` sample demonstrates using the stream classes to read and write data from a web client.

.. important::

  Any invalid data in a JSON update file will produce a debug warning, but will not cause processing to stop.
  This behaviour can be changed by implementing a custom :cpp:func:`ConfigDB::Database::handleFormatError` method.

Export
  Methods are provided to export the entire database, or a specific store, object or array.
  These can be divided into immediate methods such as :cpp:func:`ConfigDB::Database::exportToFile`,
  which are appropriate for small amounts of data or local memory or file streams.

  Methods such as :cpp:func:`ConfigDB::Database::createExportStream` create a read-only stream
  instance which is necessary for large datasets and for passing data over network connections.
  Data is serialised as it is read from the stream to minimise memory usage.

  The generated output can be customised with an optional :cpp:struct:`ConfigDB::ExportOptions` parameter.

Import
  As with export, immediate methods such as :cpp:func:`ConfigDB::Database::importFromFile`
  are available, plus streaming methods such as :cpp:func:`ConfigDB::Database::createImportStream`.

Note that both import and export use the *content* of the object and do not include the name of the object itself.
As a convenience, this behaviour can be changed via options, but only for export.


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

The **x[name=value]** syntax can be used to select *one* object from an array of objects.
Primarily this is to support data mapping with a unique *key* field, so there is the assumption that there should only be *one* matching entry in the array.

Thus only the first matching object will be selected. Matching multiple properties is not supported.

Here's the test data::

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

To delete an entry, assign to an empty array::

  {
    "x[name=object 1]": [],
    "result": [
      {
        "name": "object 2",
        "value": 2
      }
    ]
  }

If the array is populated then a delete+insert operation is performed::

  {
    "x[name=object 1]": [
      {
        "name": "object 3",
        "value": 25
      },
      {
        "name": "object 4",
        "value": 18
      }
    ],
    "result": [
      {
        "name": "object 3",
        "value": 25
      },
      {
        "name": "object 4",
        "value": 18
      },
      {
        "name": "object 2",
        "value": 2
      }
    ]
  }


.. note::

    This update expression::

      {
        "x[name=object 1]": {}
      }

    is an empty update and has no effect: It does **not** clear the object to defaults!


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
- Child objects within classes are defined as read-only member variables, such as **network.mqtt**, which is a **ContainedMqtt** class instance.
- A third *updater* class type is also generated which adds *setXXX* and *resetXXX* methods for changing values. Child objects/arrays can be updated using their provided methods.
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

.. doxygenclass:: ConfigDB::DatabaseTemplate

.. doxygenclass:: ConfigDB::Store
   :members:

.. doxygenclass:: ConfigDB::StoreRef
   :members:

.. doxygenclass:: ConfigDB::PropertyConst
   :members:

.. doxygenclass:: ConfigDB::Property
   :members:

.. doxygenclass:: ConfigDB::Object
   :members:

.. doxygenclass:: ConfigDB::ObjectTemplate
.. doxygenclass:: ConfigDB::ObjectUpdaterTemplate
.. doxygenclass:: ConfigDB::OuterObjectTemplate
.. doxygenclass:: ConfigDB::OuterObjectUpdaterTemplate

.. doxygenclass:: ConfigDB::Union
   :members:

.. doxygenclass:: ConfigDB::UnionTemplate
.. doxygenclass:: ConfigDB::UnionUpdaterTemplate

.. doxygenclass:: ConfigDB::ArrayBase
   :members:

.. doxygenclass:: ConfigDB::Array
   :members:

.. doxygenclass:: ConfigDB::ArrayTemplate
.. doxygenclass:: ConfigDB::ArrayUpdaterTemplate
.. doxygenclass:: ConfigDB::StringArrayTemplate
.. doxygenclass:: ConfigDB::StringArrayUpdaterTemplate

.. doxygenclass:: ConfigDB::ObjectArray
   :members:

.. doxygenclass:: ConfigDB::ObjectArrayTemplate
.. doxygenclass:: ConfigDB::ObjectArrayUpdaterTemplate

.. doxygenclass:: ConfigDB::Number
   :members:

.. doxygenstruct:: ConfigDB::number_t
   :members:

.. doxygenstruct:: ConfigDB::const_number_t


.. doxygenclass:: ConfigDB::Format
   :members:

.. doxygenstruct:: ConfigDB::Status
   :members:

.. doxygenclass:: ConfigDB::ImportStream
   :members:

.. doxygenclass:: ConfigDB::ExportStream
   :members:

.. doxygenstruct:: ConfigDB::ExportOptions
   :members:

.. doxygennamespace:: ConfigDB::Json
