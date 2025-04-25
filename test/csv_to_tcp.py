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
HEADER = 0
SEND_RATE_HZ = 100  # Sending rate in Hz
SHUTDOWN_CMD = "SHUTDOWN"
READY_CMD_PREFIX = "BUFFER_READY:"

# Configure logging to print to the console
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')


def pack_row(row, dtype):
    data = row.to_numpy(dtype)  # Convert pandas Series
    # to numpy array of floats
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


def send_packed_data(sock, packed_data_array, rate_hz,
                     buffer_labels=['A', 'B']):
    interval_ns = 1e9 / rate_hz  # Calculate interval between sends
    tolerance_ns = interval_ns*0.70

    for i, packed_data in enumerate(packed_data_array):
        expected_slot = buffer_labels[i % len(buffer_labels)]

        # Wait for the correct buffer to be ready
        while True:
            try:
                response = sock.recv(1024).decode().strip()
                # if response == f"{READY_CMD_PREFIX}{expected_slot}":
                #     logging.info(f"Server ready for buffer {expected_slot}")
                #     break

                break  # TODO remove

            except Exception as e:
                logging.error(f"Error receiving buffer ready message: {e}")
                return

        # logging.info(f"Interval of {interval_ns} ns") # Log the send duration
        next_send_time = time.perf_counter_ns()  # Initialize the next send time

        try:
            sock.sendall(packed_data)  # Send binary data
            next_send_time += interval_ns  # Update the next send time

            while time.perf_counter_ns() < next_send_time:
                pass

            time_behind = time.perf_counter_ns() - next_send_time
            if (time_behind > tolerance_ns):
                logging.warning(
                    f"Warning: Sending is behind schedule by {time_behind} ns")
                # break

        except Exception as e:
            logging.error(f"Error sending data for buffer {expected_slot}: {e}")
            return


def list_csv_files(directory, prefix=""):
    try:
        print(f"Checking if directory '{directory}' exists...")
        if not os.path.exists(directory):
            print(f"Directory '{directory}' does not exist.")
            return []

        all_files = os.listdir(directory)

        if not all_files:
            print(f"No files found in directory '{directory}'.")
            return []

        csv_files = [f for f in all_files if f.endswith('.csv') and
                     f.startswith(prefix)]

        if not csv_files:
            logging.info(f"No matching CSV files found in '{directory}'.")

        return csv_files
    except PermissionError:
        print(f"Permission denied for directory '{directory}'.")
        return []


def main():
    dirname = "files"
    filename_prefix = "output"  # Set to "" to include all files
    path = os.path.join(os.path.dirname(__file__), dirname)

    filename_list = list_csv_files(path, filename_prefix)
    file_path_list = [os.path.join(path, filename)
                      for filename in filename_list]

    print(f"CSV files in '{path}':")
    [print(filename) for filename in file_path_list]

    try:
        df_list = [pd.read_csv(file, delimiter=',', header=0) for file in
                   file_path_list]
    except Exception as e:
        logging.error(f"Error reading CSV files: {e}")

        return

    # Files that will be packed than sent
    # if files_to_send > len(filename_list), then
    # it executes the list circularly
    files_to_send = 2
    logging.info("Packing data")
    packed_data_list = []
    for i in range(files_to_send):
        packed_data_list.extend(pack_file(df_list[i % len(df_list)]))
    logging.info("Packing complete")
    time.sleep(2)

    try:
        # Create TCP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        logging.info("Socket open")
        sock.connect((HOST, PORT))
        logging.info("Connected")
        # send_packed_data(sock, packed_data_array)
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
