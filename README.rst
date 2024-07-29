ConfigDB
========

Sming library providing strongly typed configuration database support.


JsonSchema
----------

The database structure is defined using a standard `JSON schema <https://json-schema.org>`.

An initial schema can be created from an existing sample JSON configuration file using a generation tool such as https://github.com/saasquatch/json-schema-inferrer. Go to the online demo and paste in your JSON configuration. The resulting schema can then be edited and further customised.

The schema can then be used to provide auto-generated Standardised web editors can also be generated using tools such as https://github.com/json-editor/json-editor. Go to the online demo and scroll down to **Schema** where you can paste in

.. note::

    Sming uses JSON schema for:

        - Hardware configuration `Sming/Components/Storage/schema.json`
        - IFS build scripts `Sming/Components/IFS/tools/fsbuild/schema.json`
        - USB config `Sming/Libraries/USB/schema.json`

    It's probably fair to consider it a standard part of the framework so this should be documented somewhere centrally and referred to.


Schema rules
------------

- Root object is always a :cpp:class:`ConfigDB::Database`
- A database is always rooted in a directory
- Root object must have a `store` property indicating which class of store to use. Currently only 'json' is implemented - see :cpp:class:`ConfigDB::Json::Store`.
- Child objects may also have a `store` property. A Json store creates a separate filename using JSONPath format and does not use subdirectories.
  The name of the store forms the JSONPath prefix for any contained objects and values.

A store implementation inherits both the :cpp:class:`ConfigDB::Store` and :cpp:class:`ConfigDB::Object` classes.
Because ArduinoJson already deals with object typing, this mechanism is templated.
Other store types would need to provide type overloads as required.
