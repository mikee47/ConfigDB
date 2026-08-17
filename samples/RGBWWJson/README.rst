JSON RPC Sample
===============

Demonstrates how to process JSON RPC messages using ConfigDB.

See https://www.jsonrpc.org/specification.

Schema layout
-------------

The schema is split into three layers:

* ``value-types.cfgdb`` contains reusable scalar definitions and constraints.
* ``params.cfgdb`` contains the ``color``, ``infov1`` and ``infov2`` payload objects.
* ``color.cfgdb`` contains the tagged JSON-RPC message envelopes.

ConfigDB loads all project schemas as one set. The generated ``Params`` class owns
the payload types, while ``Color`` imports aliases for use by the JSON-RPC
envelopes.

HTTP handlers can import or export the contained ``params`` object directly, so
the HTTP request or response body has no JSON-RPC wrapper. JSON-RPC handlers use
the same object through the selected ``Color`` root variant and serialize the
complete envelope.

The top-level ``oneOf`` defines the ``color``, ``infov1`` and ``infov2`` message
tags. Each envelope requires ``jsonrpc``, ``id``, ``method`` and ``params``.

