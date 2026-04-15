import random

# Generating 10 Million rows of synthetic market data
# Format: timestamp,open,high,low,close,volume
print("Generating 10M rows of market data...")
with open("market_data.csv", "w") as f:
    f.write("timestamp,open,high,low,close,volume\n")
    ts = 1600000000
    price = 6000000 # Price in fixed-point (Price * 100)
    
    for i in range(10000000):
        o = price + random.randint(-100, 100)
        h = o + random.randint(0, 200)
        l = o - random.randint(0, 200)
        c = random.randint(l, h)
        vol = random.randint(100, 5000)
        
        f.write(f"{ts},{o},{h},{l},{c},{vol}\n")
        ts += 60 # 1-minute increments
        price = c # Next candle starts near previous close

print("Done. 'market_data.csv' created.")