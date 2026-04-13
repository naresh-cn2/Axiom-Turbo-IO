import random
import time

def generate_logs(filename, num_lines=10_000_000):
    statuses = ['200', '301', '404', '500']
    weights = [0.8, 0.05, 0.1, 0.05]
    
    print(f"Generating {num_lines} log lines...")
    start = time.time()
    
    with open(filename, 'w') as f:
        for i in range(num_lines):
            ip = f"192.168.1.{random.randint(1, 255)}"
            status = random.choices(statuses, weights)[0]
            log_line = f"{ip} - - [13/Apr/2026:09:25:01 +0530] \"GET /api/data HTTP/1.1\" {status} 512\n"
            f.write(log_line)
            
    print(f"Done in {time.time() - start:.2f} seconds.")

if __name__ == "__main__":
    generate_logs("server.log")
