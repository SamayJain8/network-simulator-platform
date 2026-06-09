# Stage 1: Build dependency packages
FROM python:3.11-slim AS builder

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir --user -r requirements.txt

# Stage 2: Runtime layer
FROM python:3.11-slim

WORKDIR /app

# Pull dependencies compiled during Stage 1
COPY --from=builder /root/.local /root/.local
COPY . .

# Set paths and ensure python output logs stream cleanly
ENV PATH=/root/.local/bin:$PATH
ENV PYTHONUNBUFFERED=1

EXPOSE 8000

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]