import os
import shutil
import datetime
import gzip


DB_PATH = "../var/lib/my_db.sqlite3"
LOG_PATH = "../var/logs/server.log"
BACKUP_DIR = "../backup"

def ensure_backup_dir():
    if not os.path.exists(BACKUP_DIR):
        os.makedirs(BACKUP_DIR, mode=0o755)

def backup_file(src_path, prefix, compress=False):
    if not os.path.exists(src_path):
        print(f"File not found: {src_path}")
        return
    date_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    base_name = f"{prefix}_{date_str}"
    dest_path = os.path.join(BACKUP_DIR, base_name)
    if compress:
        dest_path += ".gz"
        with open(src_path, "rb") as f_in, gzip.open(dest_path, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)
        print(f"Backup compressed: {dest_path}")
    else:
        shutil.copy2(src_path, dest_path)
        print(f"Backup: {dest_path}")

def main():
    ensure_backup_dir()
    backup_file(DB_PATH, "db_backup")
    backup_file(LOG_PATH, "server_log", compress=True)

if __name__ == "__main__":
    main()


"""
Automatic Backup Setup (for administrator):

To have this script run automatically every week, the administrator should:

1. Open the system's scheduled tasks file (crontab):
   $ crontab -e

2. Add the following line at the end of the file to execute the backup every Monday at 3 AM:
   0 3 * * 1 python3 path/to/backup.py

   - The first number (0) is the minute.
   - The second (3) is the hour (3 AM).
   - The third (*) is the day of the month (any day).
   - The fourth (*) is the month (any month).
   - The fifth (1) is the day of the week (1 = Monday).

3. Save and exit the editor.

This ensures the backup runs automatically every week without manual intervention.
"""