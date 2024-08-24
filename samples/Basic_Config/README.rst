Basic Config
============

Sample application demonstrating ConfigDB.

The schema
----------

The **sample-config.json** file is from the `ESP RGBWW Firmware <https://github.com/pljakobs/esp_rgbww_firmware>`__ project.
The corresponding schema **basic-config.cfgdb** was generated as follows:

- Visit https://json-schema-inferrer.herokuapp.com/
- Paste in the sample JSON file
- Select *Spec Version* **draft-07**
- Set *Policy for default* to **useFirstSamples**
- Select *Infer format for* values **ip** and **uri**
- Hit *submit* and save the generated schema with .cfgdb extension

Then do some editing:

- Open the .cfgdb file in vscode and associate with JSON format (hit *F1* then *change language mode*)
- Add ``"store": "json"`` values to the root, *general*, *color* and *events* objects.


Web client
----------

The sample starts a basic HTTP server which can be visited to view the database content in JSON format.

An HTTP POST request can also be used to update the database contents.
This must have `Content-Type: application/json` encoding.
For example, send `{"security":{"api_secured":"false"}}` to update a single value.
