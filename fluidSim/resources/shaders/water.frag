// voronoi.frag
uniform vec2 u_resolution;
uniform vec3 u_primaryColor;
uniform vec3 u_secondaryColor;
uniform vec2 u_points[10];  // Array of point positions

float voronoi(vec2 p) {
    // Find the closest point
    float minDist = 1000.0;
    
    for (int i = 0; i < 10; i++) {
        // Calculate distance to each point
        float dist = distance(p, u_points[i]);
        
        if (dist < minDist) {
            minDist = dist;
        }
    }

    return minDist;
}

float smoothValueHighLow(float x, float start, float stop) {
    if (x < start) {
        return 1.0f;
    }
    if (x > stop) {
        return 0.0f;
    }
    x = (x - start) / (stop - start);
    return 1.0f - x * x * (3.0f - x + x);
}

float smoothValueLowHigh(float x, float start, float stop) {
    if (x < start) {
        return 0.0f;
    }
    if (x > stop) {
        return 1.0f;
    }
    x = (x - start) / (stop - start);
    return x * x * (3.0f - x + x);
}

float smoothVoronoi(vec2 p, float smoothness) {
    // Find the closest two points
    float minDist1 = 1000.0f;
    float minDist2 = 1000.0f;
    
    for (int i = 0; i < 10; i++) {
        // Calculate distance to each point
        float dist = distance(p, u_points[i]);
        
        // Insert dist into distances
        if (dist < minDist1) {
            minDist2 = minDist1;
            minDist1 = dist;
        }
        else if (dist < minDist2) {
            minDist2 = dist;
        }
    }

    float diff = minDist1 - minDist2;
    if (diff <= -smoothness) {
        return minDist2;
    }
    if (diff >= smoothness) {
        return minDist1;
    }

    float h = 0.5f + (minDist1 - minDist2) / (smoothness + smoothness);
    h = smoothValueLowHigh(h, 0.0f, 1.0f);
    float value = (minDist2 * h) + (1.0f - h) * minDist1 - smoothness * h * (1.0f - h);
    return value;
}

float smoothVoronoi3(vec2 p, float smoothness) {
    // Find the closest two points
    float minDist1 = 1000.0f;
    float minDist2 = 1000.0f;
    float minDist3 = 1000.0f;
    
    for (int i = 0; i < 10; i++) {
        // Calculate distance to each point
        float dist = distance(p, u_points[i]);
        
        // Insert dist into distances
        if (dist < minDist1) {
            minDist3 = minDist2;
            minDist2 = minDist1;
            minDist1 = dist;
        }
        else if (dist < minDist2) {
            minDist3 = minDist2;
            minDist2 = dist;
        }
        else if (dist < minDist3) {
            minDist3 = dist;
        }
    }

    float smoothed = (minDist1 + minDist2 + minDist3) / 3.0f;
    return minDist1 * (1.0f - smoothness) + smoothed * smoothness;
}

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution.y;

    float smoothness = 0.6f;
    float dist = voronoi(uv) - smoothVoronoi(uv, smoothness);

    // Get color
    float colorValue = 5.0f * dist;// > 0.1f ? 1.0f : 0.0f;//smoothValue(dist, 0.0f, 0.1f);

    // Output the color of the closest point
    gl_FragColor = vec4(u_primaryColor * colorValue, 1.0);
}