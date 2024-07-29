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
- Content of database can be streamed to web clients


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
- Root object must have a **store** value (not property) indicating by namespace which backend to use.
  Currently only **json** is implemented - see :cpp:class:`ConfigDB::Json::Store`.
  All non-object configuration values live in this root store, called **_root.json**.
- Child objects may also have a **store** value attached to place them into a new store.
  These can be placed at any level in the hierarchy.
  A Json store creates a separate filename using JSONPath format and does not use subdirectories.
  The name of the store forms the JSONPath prefix for any contained objects and values.


Notes
-----

Implementations must provide both a :cpp:class:`ConfigDB::Store` and :cpp:class:`Object`.
These are placed in a common namespace; the standard implementation is :cpp:namespace:`ConfigDB::Json`.
This is identified in schema with the value **"store": "json"** on the corresponding object: See Schema Rules below.

These are virtual classes to support streaming, but the application interface uses templating to wrap ArduinoJson code.
This keeps the overhead low and should allow the compiler to optimise well.


TODO
----

- Add stream parser for receiving database content, e.g. from web clients
- Implement default value support. Thus, a default initial configuration can be created.
- Implement range checking support
- Enable custom value types. For example, can map *string* object with *ip4* format to :cpp:class:`IpAddress`.
- Implement floating point number support. The *number* type can be used for this although according to the spec. it can also contain integers.
