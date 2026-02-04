# Google Keep API Usage Documentation

## Overview

This plugin uses the **Google Keep API v1** to sync Notepad++ file contents to Google Keep notes.

## API Endpoints Used

### Create Note
```http
POST https://keep.googleapis.com/v1/notes
Content-Type: application/json
Authorization: Bearer {access_token}

{
  "title": "Note Title",
  "body": {
    "text": {
      "text": "Note content..."
    }
  },
  "labels": [
    {"name": "Notepad++"},
    {"name": "Auto-Sync"}
  ]
}
```

### Update Note
```http
PATCH https://keep.googleapis.com/v1/notes/{note_id}
Content-Type: application/json
Authorization: Bearer {access_token}

{
  "title": "Updated Title",
  "body": {
    "text": {
      "text": "Updated content..."
    }
  }
}
```

### List Notes
```http
GET https://keep.googleapis.com/v1/notes
Authorization: Bearer {access_token}
```

### Delete Note (Trash)
```http
POST https://keep.googleapis.com/v1/notes/{note_id}:trash
Authorization: Bearer {access_token}
```

## OAuth 2.0 Flow

### Authorization URL
```
https://accounts.google.com/o/oauth2/v2/auth?
  client_id={CLIENT_ID}&
  redirect_uri=http://localhost:8899/callback&
  response_type=code&
  scope=https://www.googleapis.com/auth/keep&
  access_type=offline&
  code_challenge={PKCE_CHALLENGE}&
  code_challenge_method=S256&
  state=random_state_string
```

### Token Exchange
```http
POST https://oauth2.googleapis.com/token
Content-Type: application/x-www-form-urlencoded

code={AUTHORIZATION_CODE}&
client_id={CLIENT_ID}&
client_secret={CLIENT_SECRET}&
redirect_uri=http://localhost:8899/callback&
grant_type=authorization_code&
code_verifier={PKCE_VERIFIER}
```

### Refresh Token
```http
POST https://oauth2.googleapis.com/token
Content-Type: application/x-www-form-urlencoded

refresh_token={REFRESH_TOKEN}&
client_id={CLIENT_ID}&
client_secret={CLIENT_SECRET}&
grant_type=refresh_token
```

## API Limits and Quotas

- Keep API has the following limits:
  - Requests per day: 10,000
  - Requests per 100 seconds: 1,000
  - Quota resets daily at midnight Pacific Time

## Note Properties

| Property | Type | Description |
|----------|------|-------------|
| `name` | string | Resource name (e.g., "notes/abc123") |
| `title` | string | Note title (max 1000 chars) |
| `body.text.text` | string | Note content |
| `createTime` | timestamp | Creation time (RFC 3339) |
| `updateTime` | timestamp | Last modification time |
| `trashTime` | timestamp | Time trashed (if applicable) |
| `labels[]` | array | List of labels |
| `color` | enum | Note color (RED, ORANGE, etc.) |

## Error Handling

### Common HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | None |
| 400 | Bad Request | Check request format |
| 401 | Unauthorized | Refresh/re-authenticate |
| 403 | Forbidden | Check API enabled in Cloud Console |
| 404 | Not Found | Note ID may be invalid |
| 429 | Rate Limited | Wait and retry |
| 500 | Server Error | Retry with exponential backoff |

### OAuth Errors

- `invalid_client`: Wrong Client ID/Secret
- `invalid_grant`: Expired/invalid auth code
- `unauthorized_client`: Application not authorized
- `insufficient_permissions`: Missing Keep API scope

## Security Considerations

1. **Never commit credentials** to source control
2. **Use PKCE** flow for desktop applications
3. **Store tokens securely** - encrypted if possible
4. **Implement token refresh** before expiration
5. **Use HTTPS** for all API calls
6. **Validate state parameter** in OAuth callback

## Testing

Use `curl` for manual API testing:

```bash
# Get access token
curl -X POST https://oauth2.googleapis.com/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=refresh_token" \
  -d "client_id=YOUR_CLIENT_ID" \
  -d "client_secret=YOUR_CLIENT_SECRET" \
  -d "refresh_token=YOUR_REFRESH_TOKEN"

# Create a note
curl -X POST https://keep.googleapis.com/v1/notes \
  -H "Authorization: Bearer ACCESS_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Test Note",
    "body": {"text": {"text": "Test content"}}
  }'
```

## References

- [Google Keep API Documentation](https://developers.google.com/keep/api/reference/rest)
- [OAuth 2.0 for Desktop Apps](https://developers.google.com/identity/protocols/oauth2/native-app)
- [Google Cloud Console](https://console.cloud.google.com/)
