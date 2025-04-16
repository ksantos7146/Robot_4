FROM ubuntu:latest

# Install required packages
RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake \
    libboost-all-dev \
    libasio-dev \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY Robot_4/ .

# Create build directory and compile
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

# Expose port 8080
EXPOSE 8080

# Run the application
CMD ["./build/hello_CSCN"] 