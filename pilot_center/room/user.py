"""User model for Pilot Center rooms.

This module defines a simple User class used by Room and RoomManager.

Fields:
 - user_id: unique string id for the user
 - name: optional display name
 - meta: optional dict with arbitrary metadata

The class is intentionally lightweight and thread/async friendly (no blocking
operations)."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict
import weakref
from typing import Iterable, Iterator, Optional
from .push_info import PushInfo

@dataclass
class User:
    user_id: str
    name: str = ""
    audience: bool = False
    pushers: Dict[str, PushInfo] = field(default_factory=dict)
    # weak references to session objects (WsProtooSession). Use WeakSet to
    # avoid creating strong reference cycles — WsProtooServer holds the
    # authoritative strong references to active sessions.
    def __post_init__(self) -> None:
        # Single-session binding: store one session per User. Use a weakref
        # when possible so we don't extend session lifetime accidentally.
        self._session_ref: "Optional[weakref.ref]" = None
        # fallback strong reference if object isn't weak-referenceable
        self._session_strong: "Optional[object]" = None

    def add_session(self, session: object) -> None:
        """Associate a live session with this user."""
        # Overwrite any existing session. Prefer weakref when supported.
        try:
            self._session_ref = weakref.ref(session)
            self._session_strong = None
        except Exception:
            # Not weak-referenceable: keep a strong reference (best-effort).
            self._session_ref = None
            self._session_strong = session

    def remove_session(self, session: object) -> None:
        """Remove an associated session from this user."""
        try:
            # Clear if the stored session matches the given one
            current = None
            if self._session_ref is not None:
                current = self._session_ref()
            else:
                current = self._session_strong
            if current is session:
                self._session_ref = None
                self._session_strong = None
        except Exception:
            pass

    def get_session(self) -> Optional[object]:
        """Return the single associated session, or None if not present."""
        if self._session_ref is not None:
            try:
                return self._session_ref()
            except Exception:
                return None
        return self._session_strong

    def has_sessions(self) -> bool:
        # True if a session is currently associated
        if self._session_ref is not None:
            try:
                return self._session_ref() is not None
            except Exception:
                return False
        return self._session_strong is not None

    async def send_notification_to_sessions(self, method: str, data: Optional[Dict[str, Any]] = None) -> None:
        """Async helper: send notification to all associated sessions."""
        coros = []
        s = self.get_session()
        if s is not None:
            try:
                coros.append(s.send_notification(method, data))
            except Exception:
                pass
        if coros:
            import asyncio as _asyncio

            await _asyncio.gather(*coros, return_exceptions=True)

    def to_dict(self) -> Dict[str, Any]:
        """Return a serializable representation of the user."""
        return {
            "user_id": self.user_id,
            "name": self.name,
            "pushers": {k: v.to_dict() for k, v in self.pushers.items()},
        }

    def set_pusher_info(self, info: PushInfo) -> None:
        """Set pusher info for a given media type."""
        self.pushers[info.pusherId] = info

    def get_pusher_info(self) -> Dict[str, Any]:
        """Get all pusher info as a dict."""
        return self.pushers

    def get_pusher_by_id(self, pusherId: str) -> Any:
        """Get pusher info by pusher ID."""
        return self.pushers.get(pusherId)

    def clear_pusher_info(self) -> None:
        """Clear all pusher info."""
        self.pushers.clear()

    def get_user_id(self) -> str:
        """Return the user ID."""
        return self.user_id
    def get_name(self) -> str:
        """Return the user name."""
        return self.name
    def IsAudience(self) -> bool:
        """Return whether the user is an audience member."""
        return self.audience
    def SetAudience(self, audience: bool) -> None:
        """Set whether the user is an audience member."""
        self.audience = audience
