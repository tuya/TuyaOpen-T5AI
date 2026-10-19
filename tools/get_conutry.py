import datetime
import os


def get_country_code():
    global MORROR
    if MORROR == 0:
        # Prefer the region the SDK already detected; see tools/util.py.
        code = os.environ.get("OPEN_COUNTRY_CODE", "")
        if code:
            MORROR = 1 if code == "China" else 2
        else:
            try:
                offset = datetime.datetime.now().astimezone().utcoffset()
                seconds = offset.total_seconds() if offset is not None else 0
                MORROR = 1 if int(seconds) == 8 * 3600 else 2
            except Exception:
                MORROR = 2

    print(MORROR)


if __name__ == "__main__":
    MORROR = 0
    get_country_code()
