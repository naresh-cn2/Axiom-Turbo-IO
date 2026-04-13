import re
import time

def extract_404_ips(filename):
    start = time.time()
    pattern = re.compile(r'^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}).*\" 404 ')
    error_ips = []
    
    with open(filename, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                error_ips.append(match.group(1))
                
    total_time = time.time() - start
    print(f"Found {len(error_ips)} 404 errors.")
    print(f"Slow Python Time: {total_time:.2f} seconds")

if __name__ == "__main__":
    extract_404_ips("server.log")
