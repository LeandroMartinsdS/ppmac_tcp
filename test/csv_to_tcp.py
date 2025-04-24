import os
import socket
import struct
import time
import logging
import pandas as pd

# Define the server address and port
HOST = '127.0.0.1'  # Replace with your MCU's IP address
PORT = 8080

DATA_FORMAT = '<7d'  # Format for 6 double variables - forces little-endian
SEND_RATE_HZ = 1000  # Sending rate in Hz
SHUTDOWN_CMD = "SHUTDOWN"

def pack_row(row, dtype):
    data = row.to_numpy(dtype)  # Convert pandas Series to numpy array of floats
    return struct.pack(DATA_FORMAT, *data)  # Convert to binary

def pack_file(df, dtype=float):
    packed_data_list = []
    for i, row in df.iterrows():
        # Processing row
        try:
            if row.isnull().all():
                # Skip empty rows
                continue
            packed_data_list.append(pack_row(row, dtype))
        except ValueError as e:
            logging.error(f"Error converting row {i} to floats: {e}")
    return packed_data_list

# Configure logging to print to the console
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

def send_packed_data(sock, packed_data_array, rate_hz):
    interval_ns = 1e9 / rate_hz  # Calculate interval between sends
    tolerance_ns = interval_ns*0.50

    # logging.info(f"Interval of {interval_ns} ns") # Log the send duration
    next_send_time = time.perf_counter_ns()  # Initialize the next send time

    for packed_data in packed_data_array:
        # start_time = time.perf_counter_ns()
        sock.sendall(packed_data)  # Send binary data
        # end_time = time.perf_counter_ns()
        # send_duration = end_time - start_time
        # logging.info(f"Packet sent in {send_duration} ns") # Log the send time

        next_send_time += interval_ns  # Update the next send time


        while time.perf_counter_ns() < next_send_time:
            pass

        time_behind = time.perf_counter_ns() - next_send_time
        if (time_behind > tolerance_ns):
            logging.warning(f"Warning: Sending is behind schedule by {time_behind} ns")
            # break


def list_csv_files(directory):
    try:
        print(f"Checking if directory '{directory}' exists...")
        if not os.path.exists(directory):
            print(f"Directory '{directory}' does not exist.")
            return []

        print(f"Listing all files in directory '{directory}'...")
        all_files = os.listdir(directory)
        print(f"All files: {all_files}")

        if not all_files:
            print(f"No files found in directory '{directory}'.")
            return []

        print(f"Filtering CSV files...")
        csv_files = [f for f in all_files if f.endswith('.csv')]
        print(f"CSV files: {csv_files}")

        if not csv_files:
            print(f"No CSV files found in directory '{directory}'.")

        return csv_files
    except PermissionError:
        print(f"Permission denied for directory '{directory}'.")
        return []


print("Starting script...")  # Confirm script execution
path = os.path.join(os.path.dirname(__file__), "files")
print(f"Constructed path: {path}")  # Print constructed path
filename_list = list_csv_files(path)
file_path_list = filename_list
print(f"CSV files in '{path}': {file_path_list}")


def main():

    path = os.path.join(os.path.dirname(__file__),"files")
    print(f"Constructed path: {path}")  # Print constructed path
    filename_list = list_csv_files(path)
    print(f"CSV files in '{path}': {filename_list}")
    file_path_list = [os.path.join(path,filename) for filename in filename_list]
    [print(filename) for filename in file_path_list]

    try:
        df = [pd.read_csv(file, delimiter=',') for file in file_path_list]
    except Exception as e:
        logging.error(f"Error reading CSV files: {e}")

        return

    # Files that will be packed than sent if files_to_send > len(filename_list),
    # then it executes the list circularly
    files_to_send = 3
    logging.info("Packing data")
    packed_data_list=[]
    for i in range(files_to_send):
        packed_data_list.extend(pack_file(df[i % len(df)]))
    logging.info("Packing complete")
    time.sleep(2)

    try:
        # Create TCP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        logging.info("Socket open")
        sock.connect((HOST, PORT))
        logging.info("Connected")
        #send_packed_data(sock, packed_data_array)
        send_packed_data(sock, packed_data_list, SEND_RATE_HZ)
        shutdown_command = SHUTDOWN_CMD.encode('utf-8')
        sock.sendall(shutdown_command)
        logging.info("End of communication")
    except Exception as e:
        logging.error(f"Error in socket communication: {e}")
    finally:
        sock.close()

if __name__ == '__main__':
    main()