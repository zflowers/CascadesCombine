# file_lock.py
import os
import time
import fcntl

# --- Simple advisory file lock utility (fcntl.flock) ---

LOCK_FILENAME = os.path.join("runs", ".build_and_stage.lock")

class FileLockTimeout(Exception):
    """Raised when FileLock.acquire() times out without acquiring the lock."""
    pass


class FileLock:
    """
    Advisory file lock using fcntl.flock. This is cooperative between processes
    that use the same lock file.

    Usage:
        from file_lock import FileLock, FileLockTimeout

        lock = FileLock(LOCK_FILENAME)
        # non-blocking attempt:
        if not lock.try_acquire():
            # someone else holds the lock
            ...
        # or blocking with optional timeout:
        lock.acquire(timeout=300)   # wait up to 300s
        try:
            # critical section
            ...
        finally:
            lock.release()
    """

    def __init__(self, lock_path: str = LOCK_FILENAME):
        self.lock_path = lock_path
        # ensure parent directory exists
        parent = os.path.dirname(self.lock_path) or "."
        os.makedirs(parent, exist_ok=True)
        self._fd = None

    def _open(self):
        if self._fd is None:
            # open in append so we can write pid/timestamp info
            self._fd = open(self.lock_path, "a+")
        return self._fd

    def try_acquire(self) -> bool:
        """Try to acquire lock non-blocking. Return True if acquired, False otherwise."""
        fd = self._open()
        try:
            fcntl.flock(fd.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            # write some debug info
            try:
                fd.seek(0)
                fd.truncate()
                fd.write(f"PID {os.getpid()} acquired lock at {time.ctime()}\n")
                fd.flush()
                os.fsync(fd.fileno())
            except Exception:
                pass
            return True
        except BlockingIOError:
            return False

    def acquire(self, timeout: float = None, poll_interval: float = 1.0):
        """
        Acquire lock, blocking. If timeout is provided, raise FileLockTimeout on timeout.
        """
        fd = self._open()
        start = time.time()
        while True:
            try:
                fcntl.flock(fd.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                try:
                    fd.seek(0)
                    fd.truncate()
                    fd.write(f"PID {os.getpid()} acquired lock at {time.ctime()}\n")
                    fd.flush()
                    os.fsync(fd.fileno())
                except Exception:
                    pass
                return True
            except BlockingIOError:
                if timeout is not None and (time.time() - start) >= timeout:
                    raise FileLockTimeout(f"Timeout acquiring lock {self.lock_path}")
                time.sleep(poll_interval)

    def release(self):
        """Release the lock and close the file descriptor."""
        if self._fd:
            try:
                fcntl.flock(self._fd.fileno(), fcntl.LOCK_UN)
            except Exception:
                pass
            try:
                self._fd.close()
            except Exception:
                pass
            self._fd = None

    # context manager convenience
    def __enter__(self):
        self.acquire()
        return self

    def __exit__(self, exc_type, exc, tb):
        self.release()
