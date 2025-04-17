# Use a general-purpose base image for development
FROM ubuntu:latest AS developer

# Update package lists
RUN apt-get update

# Install necessary development packages
RUN apt-get install -y build-essential libc6-dev linux-headers-generic

# Create symbolic link for sys directory
RUN ln -s /usr/include/x86_64-linux-gnu/sys /usr/include/sys

# Install system dependencies for both Python and C/C++ development
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-dev \
    python3-venv \
    python3-pip \
    gcc \
    g++ \
    gdb \
    linux-headers-generic \
    make \
    git \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Set up a virtual environment for Python and put it in PATH
RUN python3 -m venv /venv
ENV PATH=/venv/bin:$PATH

# The build stage installs the context into the container
FROM developer AS build
COPY . /context
WORKDIR /context

# Upgrade pip to ensure the latest version is used
RUN pip install --upgrade pip
# Install Python dependencies
RUN pip install pandas socket

# The runtime stage copies the built context into a slim runtime container
FROM ubuntu:latest AS runtime
COPY --from=build /context/ /context/
COPY --from=build /venv/ /venv/
ENV PATH=/venv/bin:$PATH
WORKDIR /context

# Change this entrypoint if it is not the same as the repo
CMD ["--version"]
