# OpenAPI Specification

This directory contains the OpenAPI 3.0 specification for the ESP32 MMDVM Hotspot API.

## File

**swagger.json** - Complete OpenAPI 3.0 specification with all endpoints, schemas, and examples

## What's Included

- All 11 API endpoints fully documented
- Request/response schemas with examples
- HTTP Basic Authentication details
- Error response formats
- Data type definitions

## How to Use

### Swagger UI (Online)
1. Visit [Swagger Editor](https://editor.swagger.io/)
2. File → Import File → Choose `swagger.json`
3. View interactive documentation
4. Test endpoints directly (configure auth first)

### Postman
1. Open Postman
2. Import → Upload Files → Choose `swagger.json`
3. Collection will be created with all endpoints
4. Configure environment variables and auth

### Insomnia
1. Open Insomnia
2. Create → Import From → File → Choose `swagger.json`
3. Workspace will be populated with requests

### Code Generation
Generate API client code in various languages:

```bash
# Install OpenAPI Generator
npm install -g @openapitools/openapi-generator-cli

# Generate Python client
openapi-generator-cli generate -i swagger.json -g python -o ./python-client

# Generate JavaScript client
openapi-generator-cli generate -i swagger.json -g javascript -o ./js-client

# Generate Go client
openapi-generator-cli generate -i swagger.json -g go -o ./go-client
```

### Self-Hosted Swagger UI

Host your own interactive API documentation:

```bash
# Using Docker
docker run -p 8080:8080 \
  -e SWAGGER_JSON=/swagger.json \
  -v $(pwd)/swagger.json:/swagger.json \
  swaggerapi/swagger-ui

# Access at http://localhost:8080
```

## API Endpoints Overview

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/status` | GET | Network & DMR status |
| `/api/system-information` | GET | ESP32 hardware details |
| `/api/modem-information` | GET | MMDVM modem info |
| `/api/dmr-activity` | GET | Live DMR activity (both slots) |
| `/api/dmr-slot1` | GET | DMR Slot 1 activity |
| `/api/dmr-slot2` | GET | DMR Slot 2 activity |
| `/api/dmr-history` | GET | Recent DMR transmissions |
| `/api/rf-history` | GET | Local RF activity |
| `/api/system-status` | GET | System status overview |
| `/api/logs` | GET | Serial monitor logs |
| `/api/wifiscan` | GET | Available WiFi networks |

All endpoints require HTTP Basic Authentication.

## Documentation

For complete API documentation with examples, see [../../API_README.md](../../API_README.md)

For Node-RED integration examples, see [../node-red/](../node-red/)

---

**73 de PD2EMC** 
