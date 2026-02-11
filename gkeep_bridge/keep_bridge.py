#!/usr/bin/env python3
"""
⛔ ARCHIVED - Google Keep Bridge for Notepad++ Plugin

ARCHIVED: Google deprecated Keep API access for personal accounts (Jan 2025)

This module provided a bridge between Notepad++ and Google Keep using gkeepapi.
Authentication is now broken due to Google's deprecation of third-party access.
"""

import sys
import os
import json
import base64
from pathlib import Path
from typing import Dict, List, Optional, Any

try:
    import gkeepapi
    from gkeepapi.exception import LoginException
    from gpsoauth import perform_master_login, exchange_token
except ImportError:
    print(json.dumps({"error": "gkeepapi and gpsoauth required. Run: pip install gkeepapi gpsoauth"}), file=sys.stderr)
    sys.exit(1)


class KeepBridge:
    """Bridge between Notepad++ plugin and Google Keep API."""
    
    # Default Android ID for Google Keep
    ANDROID_ID = "ae7d752d1764a7b6"
    
    def __init__(self):
        self.keep = gkeepapi.Keep()
        self.config_dir = self._get_config_dir()
        self.auth_file = self.config_dir / "auth.json"
        self.state_file = self.config_dir / "state.bin"
        self.email: Optional[str] = None
        self.master_token: Optional[str] = None
        self.device_id: Optional[str] = None
        
    def _get_config_dir(self) -> Path:
        """Get configuration directory for storing auth data."""
        if os.name == 'nt':
            appdata = os.environ.get('APPDATA')
            if appdata:
                config_dir = Path(appdata) / "Notepad++" / "plugins" / "config" / "keep_bridge"
            else:
                config_dir = Path.home() / ".keep_bridge"
        else:
            config_dir = Path.home() / ".config" / "keep_bridge"
        config_dir.mkdir(parents=True, exist_ok=True)
        return config_dir
    
    def _load_auth(self) -> bool:
        try:
            if self.auth_file.exists():
                with open(self.auth_file, 'r') as f:
                    auth_data = json.load(f)
                    self.email = auth_data.get('email')
                    self.master_token = auth_data.get('master_token')
                    self.device_id = auth_data.get('device_id')
                    return True
            return False
        except Exception:
            return False
    
    def _save_auth(self) -> bool:
        try:
            auth_data = {
                'email': self.email,
                'master_token': self.master_token,
                'device_id': self.device_id
            }
            with open(self.auth_file, 'w') as f:
                json.dump(auth_data, f)
            return True
        except Exception:
            return False
    
    def _save_state(self) -> bool:
        try:
            self.keep.dump(self.state_file)
            return True
        except Exception:
            return False
    
    def _load_state(self) -> bool:
        try:
            if self.state_file.exists():
                self.keep.restore(self.state_file)
                return True
            return False
        except Exception:
            return False
    
    def handle_login(self, params: Dict[str, Any]) -> Dict[str, Any]:
        email = params.get('email')
        app_password = params.get('app_password')
        if not email or not app_password:
            return {"success": False, "error": "Email and app_password required"}
        
        try:
            self.email = email
            
            # Step 1: Exchange credentials for master token
            android_id = params.get('android_id', self.ANDROID_ID)
            
            # Get master token from exchange
            master_response = exchange_token(email, app_password, android_id)
            if 'Token' not in master_response:
                # Check if we have a cached token we can try
                if self._load_auth() and self.master_token:
                    print("Trying cached master token...", file=sys.stderr)
                    self.keep.authenticate(email, self.master_token, self.device_id)
                    self._save_state()
                    return {"success": True, "message": "Login successful (cached token)", "email": email}
                return {"success": False, "error": f"Failed to get master token: {master_response.get('Error', 'Unknown error')}"}
            
            self.master_token = master_response['Token']
            
            # Step 2: Use master token to login
            login_response = perform_master_login(email, self.master_token, android_id)
            if 'Auth' not in login_response:
                return {"success": False, "error": f"Login failed: {login_response.get('Error', 'Unknown error')}"}
            
            # Extract device_id from Auth response
            auth = login_response.get('Auth', '')
            if '=' in auth:
                self.device_id = auth.split('=')[1]
            else:
                self.device_id = android_id
            
            # Step 3: Authenticate with gkeepapi
            self.keep.authenticate(email, self.master_token, self.device_id)
            
            # Save auth for future use
            self._save_auth()
            self._save_state()
            
            return {"success": True, "message": "Login successful", "email": email}
        except LoginException as e:
            return {"success": False, "error": f"Login failed: {str(e)}"}
        except Exception as e:
            return {"success": False, "error": f"Unexpected error: {str(e)}"}
    
    def handle_status(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """Check if authenticated and get status."""
        # Try to load cached auth
        has_auth = self._load_auth()
        
        if has_auth and self.master_token:
            try:
                # Try to use cached token
                if self.email:
                    self.keep.authenticate(self.email, self.master_token, self.device_id)
                return {
                    "authenticated": True,
                    "email": self.email,
                    "has_cached_token": True,
                    "message": "Authenticated (cached token)"
                }
            except Exception:
                pass
        
        return {
            "authenticated": False,
            "email": self.email,
            "has_cached_token": has_auth,
            "message": "Not authenticated"
        }
    
    def handle_sync(self, params: Dict[str, Any]) -> Dict[str, Any]:
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        try:
            self._load_state()
            self.keep.sync()
            self._save_state()
            return {"success": True, "message": "Sync completed"}
        except Exception:
            # Try re-authenticating on sync failure
            try:
                self.keep.authenticate(self.email, self.master_token, self.device_id)
                self.keep.sync()
                self._save_state()
                return {"success": True, "message": "Sync completed after re-auth"}
            except Exception as e2:
                return {"success": False, "error": f"Sync failed: {str(e2)}"}
    
    def handle_list(self, params: Dict[str, Any]) -> Dict[str, Any]:
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        try:
            self._load_state()
            all_notes = params.get('all', False)
            limit = params.get('limit', 100)
            query = params.get('query', '')
            labels = params.get('labels', [])
            notes_list = []
            notes = self.keep.find(query=query) if query else self.keep.all()
            count = 0
            for note in notes:
                if count >= limit:
                    break
                if note.archived and not all_notes:
                    continue
                if labels:
                    note_labels = [l.name for l in note.labels.all()]
                    if not any(l in note_labels for l in labels):
                        continue
                notes_list.append({
                    "id": note.id,
                    "title": note.title or "Untitled",
                    "text": note.text[:500] if note.text else "",
                    "pinned": note.pinned,
                    "archived": note.archived,
                    "color": note.color.name if note.color else "UNKNOWN",
                    "labels": [l.name for l in note.labels.all()],
                    "timestamp": str(note.timestamps.created) if note.timestamps else ""
                })
                count += 1
            return {"success": True, "count": len(notes_list), "notes": notes_list}
        except Exception as e:
            return {"success": False, "error": f"List failed: {str(e)}"}
    
    def handle_get(self, params: Dict[str, Any]) -> Dict[str, Any]:
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        note_id = params.get('id')
        if not note_id:
            return {"success": False, "error": "Note ID required"}
        try:
            self._load_state()
            note = self.keep.get(note_id)
            if not note:
                return {"success": False, "error": "Note not found"}
            result = {
                "success": True,
                "note": {
                    "id": note.id,
                    "title": note.title or "Untitled",
                    "text": note.text or "",
                    "pinned": note.pinned,
                    "archived": note.archived,
                    "color": note.color.name if note.color else "UNKNOWN",
                    "labels": [l.name for l in note.labels.all()],
                    "timestamps": {
                        "created": str(note.timestamps.created) if note.timestamps else "",
                        "edited": str(note.timestamps.edited) if note.timestamps else "",
                        "updated": str(note.timestamps.updated) if note.timestamps else ""
                    }
                }
            }
            return result
        except Exception as e:
            return {"success": False, "error": f"Get failed: {str(e)}"}
    
    def handle_delete(self, params: Dict[str, Any]) -> Dict[str, Any]:
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        note_id = params.get('id')
        permanent = params.get('permanent', False)
        if not note_id:
            return {"success": False, "error": "Note ID required"}
        try:
            self._load_state()
            note = self.keep.get(note_id)
            if not note:
                return {"success": False, "error": "Note not found"}
            if permanent:
                note.delete()
                message = "Note permanently deleted"
            else:
                note.archived = True
                message = "Note archived"
            self.keep.sync()
            self._save_state()
            return {"success": True, "message": message, "id": note_id}
        except Exception as e:
            return {"success": False, "error": f"Delete failed: {str(e)}"}
    
    def handle_create_note(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """Create a new note in Google Keep"""
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        
        title = params.get('title', '')
        text = params.get('text', '')
        pinned = params.get('pinned', False)
        color = params.get('color', 'DEFAULT')
        labels = params.get('labels', [])
        
        if not title and not text:
            return {"success": False, "error": "Note must have title or text"}
        
        try:
            self._load_state()
            note = self.keep.createNote(title, text)
            note.pinned = pinned
            
            # Map color string to gkeepapi color
            if color != 'DEFAULT':
                try:
                    color_enum = getattr(gkeepapi.node.ColorValue, color.upper())
                    note.color = color_enum
                except AttributeError:
                    pass
            
            # Add labels if any
            for label_name in labels:
                label = self.keep.findLabel(label_name)
                if not label:
                    label = self.keep.createLabel(label_name)
                note.labels.add(label)
            
            self.keep.sync()
            self._save_state()
            
            return {
                "success": True,
                "message": "Note created",
                "note": {
                    "id": note.id,
                    "title": note.title,
                    "text": note.text,
                    "pinned": note.pinned,
                    "color": note.color.name if note.color else 'DEFAULT',
                    "labels": [label.name for label in note.labels],
                    "timestamps": {
                        "created": str(note.timestamps.created),
                        "edited": str(note.timestamps.edited)
                    }
                }
            }
        except Exception as e:
            return {"success": False, "error": f"Failed to create note: {str(e)}"}
    
    def handle_update_note(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """Update an existing note in Google Keep"""
        if not self._load_auth():
            return {"success": False, "error": "Not authenticated. Login first."}
        
        note_id = params.get('id')
        title = params.get('title')
        text = params.get('text')
        pinned = params.get('pinned')
        color = params.get('color')
        labels = params.get('labels')
        
        if not note_id:
            return {"success": False, "error": "Note ID required"}
        
        try:
            self._load_state()
            note = self.keep.get(note_id)
            if not note:
                return {"success": False, "error": "Note not found"}
            
            if title is not None:
                note.title = title
            if text is not None:
                note.text = text
            if pinned is not None:
                note.pinned = pinned
            if color is not None:
                try:
                    if color == 'DEFAULT':
                        note.color = None
                    else:
                        color_enum = getattr(gkeepapi.node.ColorValue, color.upper())
                        note.color = color_enum
                except AttributeError:
                    pass
            
            # Update labels if provided
            if labels is not None:
                note.labels.clear()
                for label_name in labels:
                    label = self.keep.findLabel(label_name)
                    if not label:
                        label = self.keep.createLabel(label_name)
                    note.labels.add(label)
            
            self.keep.sync()
            self._save_state()
            
            return {
                "success": True,
                "message": "Note updated",
                "note": {
                    "id": note.id,
                    "title": note.title,
                    "text": note.text,
                    "pinned": note.pinned,
                    "color": note.color.name if note.color else 'DEFAULT',
                    "labels": [label.name for label in note.labels],
                    "timestamps": {
                        "created": str(note.timestamps.created),
                        "edited": str(note.timestamps.edited)
                    }
                }
            }
        except Exception as e:
            return {"success": False, "error": f"Failed to update note: {str(e)}"}
    
    def handle_set_token(self, params: Dict[str, Any]) -> Dict[str, Any]:
        """Debug: Manually set master token. Use this if you obtained a token externally."""
        email = params.get('email')
        master_token = params.get('master_token')
        device_id = params.get('device_id', self.ANDROID_ID)
        
        if not email or not master_token:
            return {"success": False, "error": "email and master_token required"}
        
        try:
            self.email = email
            self.master_token = master_token
            self.device_id = device_id
            
            # Test the token
            self.keep.authenticate(email, master_token, device_id)
            
            # Save
            self._save_auth()
            self._save_state()
            
            return {"success": True, "message": "Token saved and validated", "email": email}
        except Exception as e:
            return {"success": False, "error": f"Token validation failed: {str(e)}"}
    
    def process_command(self, command: Dict[str, Any]) -> Dict[str, Any]:
        cmd = command.get('command')
        params = command.get('params', {})
        handlers = {
            'login': self.handle_login,
            'sync': self.handle_sync,
            'list': self.handle_list,
            'get': self.handle_get,
            'delete': self.handle_delete,
            'create_note': self.handle_create_note,
            'update_note': self.handle_update_note,
            'status': self.handle_status,
            'set_token': self.handle_set_token  # Debug: manually set master token
        }
        handler = handlers.get(cmd)
        if handler:
            return handler(params)
        return {"success": False, "error": f"Unknown command: {cmd}"}
    
    def run(self):
        while True:
            try:
                line = sys.stdin.readline()
                if not line:
                    break
                line = line.strip()
                if not line:
                    continue
                try:
                    command = json.loads(line)
                    result = self.process_command(command)
                    print(json.dumps(result), flush=True)
                except json.JSONDecodeError as e:
                    print(json.dumps({"success": False, "error": f"Invalid JSON: {str(e)}"}), flush=True)
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(json.dumps({"success": False, "error": f"Internal error: {str(e)}"}), flush=True)


def main():
    bridge = KeepBridge()
    bridge.run()


if __name__ == '__main__':
    main()
