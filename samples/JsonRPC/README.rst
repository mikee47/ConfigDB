JSON RPC Sample
===============

Demonstrates how to process JSON RPC messages using ConfigDB.

See https://www.jsonrpc.org/specification.

Messages cannot be fully assembled via ConfigDB as the content is variable.
All messages require a standard `"jsonrpc": "2.0"` and `id` property (except notifications).
A request requires `method` and `params` properties.
A Response requires `result` on success, and `error` on failure.

The `params` and `result` fields are application-specific and can be complex,
which is where ConfigDB can be helpful.

For parsing, it is necessary to first scan the message to extract the standard fields
and establish what kind of message it is. The `params` and `result` can then be processed via ConfigDB.

The ConfigDB schema uses `oneOf` to define the various request messages.

