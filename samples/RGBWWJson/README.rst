JSON RPC Sample
===============

Demonstrates how to generate JSON-RPC 2.0 messages with ConfigDB while
reusing the same parameter objects for an HTTP API.

See https://www.jsonrpc.org/specification.

Schema layout
-------------

The schema is split into three layers:

* ``value-types.cfgdb`` contains reusable scalar definitions, constants and
	constraints. This includes the constant JSON-RPC version ``"2.0"`` and the
	three method names.
* ``params.cfgdb`` contains the reusable parameter objects for ``color``,
	``infov1`` and ``infov2``. These objects are independent of any transport.
* ``jsonrpc.cfgdb`` adds the JSON-RPC envelope around those parameter objects.



