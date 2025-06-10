with open("city_new.bin", "rb") as f:
    data = f.read()

# GBMP = \x47\x42\x4d\x50 (ASCII value correspondence)
# PK = \x50\x4B

count = data.count(b"GBMP")
new_data = data.replace(b"GBMP", b"PK\x03\x04")

with open("city_new_fixed.bin", "wb") as f:
    f.write(new_data)

if count:
    print(f"Header found and replaced {count} times.")
else:
    print("Header not found, no replacements made.")
