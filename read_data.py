from pprint import pprint
def leer():
    data = []
    with open("/media/udmt/canlog/log.bin", "rb") as f:
        raw = f.read().hex()
        
        for i in range(0, len(raw), 12*2):
            data.append(raw[i:i+12*2])
    return data


def decode_data(raw_data: list[str]):
    data = []
    for i in raw_data:
        data.append({
            "timestamp": int(i[0:8], 16),
            "data": i[8:]
        })
    return data

if __name__ == "__main__":
    raw_data = leer()
    data = decode_data(raw_data)
    pprint(data)