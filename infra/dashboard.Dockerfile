# Stage 1: Compile the production React build using Vite
FROM node:18-alpine AS builder

WORKDIR /app
COPY package.json .
RUN npm install --silent

COPY . .
RUN npm run build

# Stage 2: Deploy to Nginx web-server
FROM nginx:1.25-alpine

# Copy static assets from Vite's output directory
COPY --from=builder /app/dist /usr/share/nginx/html
EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]