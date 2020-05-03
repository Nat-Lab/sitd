`sitd` API Reference
---

`sitd` utilizes a simple restful interface for tunnel management.

## API Payloads

### Error Payload

This payload is returned when a method fails (HTTP 4xx/5xx).

field|type|description
--|--|--
message|string|error message.
code|enum `ErrorCode`|error code.

### Tunnel Payload

field|type|description
--|--|--
state|enum `TunnelState`|tunnel state
remote|string|remote IP address.
local|string|local IP address.
address|string|IPv6 address on the SIT interface.
mtu?|number|tunnel MTU. (default: auto)

## Enums

### ErrorCode

code|description
--|--
ERR_UNKNOW|unknow error.
ERR_BAD_STATE|invalid tunnel state.
ERR_BAD_LOCAL|invalid local address.
ERR_BAD_REMOTE|invalid remote address.
ERR_BAD_ADDRESS|invalid IPv6 interface address.
ERR_BAD_MTU|invalid MTU.

### TunnelState

state|description
--|--
stopped|tunnel stopped.
running|tunnel running.
restarting|restarts the tunnel: this state is for requesting tunnel restart only and will never show up in API response. 
reloading|reloads the tunnel: this state is for requesting tunnel reload only and will never show up in API response. 




## API Methods

`sitd` provides an easy-to-use RESTful API. This document outlines the available RESTful methods. 

URL: `/api/v1/tunnel/:name`

- `GET` retrieves information about an existing tunnel.
- `POST` creates a new tunnel.
- `PUT` updates an existing tunnel.
- `DELETE` removes an existing tunnel.

### Get Tunnel Information

This method will return a payload containing details about the tunnel.

- __Method__: `GET`
- __Request__: `NONE`
- __Respond__: `Tunnel`

### Create Tunnel

This method will return a new tunnel with details in the request. This method will then return a payload containing information about the newly created tunnel.

- __Method__: `POST`
- __Request__: `Tunnel`
- __Respond__: `Tunnel`

### Modify Tunnel

This method will modify the details of tunnel with details in the request. This method will then return a payload containing information about the edited tunnel.

- __Method__: `PUT`
- __Request__: `Tunnel`
- __Respond__: `Tunnel`

### Delete Tunnel

This method will delete an existing tunnel. This method will then return a payload containing information about the deleted tunnel just before deletion.

- __Method__: `DELETE`
- __Request__: `NONE`
- __Respond__: `Tunnel`