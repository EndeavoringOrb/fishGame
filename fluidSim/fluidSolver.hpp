// FluidSolver.hpp
#pragma once

#define SWAP(x0, x)      \
    {                    \
        float *tmp = x0; \
        x0 = x;          \
        x = tmp;         \
    }

#include <algorithm>
#include <cstring>
#include <vector>

struct Source
{
    int x, y;
    float amount0;
    float amount1;
    bool densitySource;

    Source(int _x, int _y, float _amount) : x(_x), y(_y), amount0(_amount)
    {
        densitySource = true;
    }

    Source(int _x, int _y, float _uAmount, float _vAmount) : x(_x), y(_y), amount0(_uAmount), amount1(_vAmount)
    {
        densitySource = false;
    }
};

class FluidSolver
{
public:
    int N;
    int size;
    float visc;
    float diff;
    float dt;

    float *u;
    float *v;
    float *dens;
    float *u_prev;
    float *v_prev;
    float *dens_prev;
    std::vector<Source> sources;

    FluidSolver(int _N, float _visc, float _diff, float _dt);
    ~FluidSolver();

    int IX(int i, int j);
    void addSource(Source source);
    void addSource(int N, float *x, float *s, float dt);
    void diffuse(int N, int b, float *x, float *x0, float diff, float dt);
    void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt);
    void project(int N, float *u, float *v, float *p, float *div);
    void setBoundary(int N, int b, float *x);
    void addSources(bool density);
    sf::Vector2f densityBounds();

    void densStep(int N, float *x, float *x0, float *u, float *v, float diff, float dt);
    void velStep(int N, float *u, float *v, float *u0, float *v0, float visc, float dt);

    void clearPrev();
    void clear();
    void update();
};

FluidSolver::FluidSolver(int _N, float _visc, float _diff, float _dt)
    : N(_N), visc(_visc), diff(_diff), dt(_dt)
{
    size = (N + 2) * (N + 2);

    u = new float[size]();
    v = new float[size]();
    dens = new float[size]();
    u_prev = new float[size]();
    v_prev = new float[size]();
    dens_prev = new float[size]();
}

FluidSolver::~FluidSolver()
{
    delete[] u;
    delete[] v;
    delete[] dens;
    delete[] u_prev;
    delete[] v_prev;
    delete[] dens_prev;
}

int FluidSolver::IX(int i, int j)
{
    return i + (N + 2) * j;
}

void FluidSolver::addSource(int N, float *x, float *s, float dt)
{
    for (int i = 0; i < size; i++)
    {
        x[i] += dt * s[i];
    }
}

void FluidSolver::diffuse(int N, int b, float *x, float *x0, float diff, float dt)
{
    float a = dt * diff * N * N;

    for (int k = 0; k < 20; k++)
    {
        for (int i = 1; i <= N; i++)
        {
            for (int j = 1; j <= N; j++)
            {
                x[IX(i, j)] = (x0[IX(i, j)] + a * (x[IX(i - 1, j)] + x[IX(i + 1, j)] +
                                                   x[IX(i, j - 1)] + x[IX(i, j + 1)])) /
                              (1 + 4 * a);
            }
        }
        setBoundary(N, b, x);
    }
}

void FluidSolver::advect(int N, int b, float *d, float *d0, float *u, float *v, float dt)
{
    float dt0 = dt * N;

    for (int i = 1; i <= N; i++)
    {
        for (int j = 1; j <= N; j++)
        {
            float x = i - dt0 * u[IX(i, j)];
            float y = j - dt0 * v[IX(i, j)];

            x = std::max(0.5f, std::min(x, N + 0.5f));
            y = std::max(0.5f, std::min(y, N + 0.5f));

            int i0 = static_cast<int>(x);
            int j0 = static_cast<int>(y);
            int i1 = i0 + 1;
            int j1 = j0 + 1;

            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;

            d[IX(i, j)] = s0 * (t0 * d0[IX(i0, j0)] + t1 * d0[IX(i0, j1)]) +
                          s1 * (t0 * d0[IX(i1, j0)] + t1 * d0[IX(i1, j1)]);
        }
    }

    setBoundary(N, b, d);
}

void FluidSolver::project(int N, float *u, float *v, float *p, float *div)
{
    float h = 1.0f / N;

    for (int i = 1; i <= N; i++)
    {
        for (int j = 1; j <= N; j++)
        {
            div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] + v[IX(i, j + 1)] - v[IX(i, j - 1)]);
            p[IX(i, j)] = 0;
        }
    }

    setBoundary(N, 0, div);
    setBoundary(N, 0, p);

    for (int k = 0; k < 20; k++)
    {
        for (int i = 1; i <= N; i++)
        {
            for (int j = 1; j <= N; j++)
            {
                p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] +
                               p[IX(i, j - 1)] + p[IX(i, j + 1)]) /
                              4;
            }
        }
        setBoundary(N, 0, p);
    }

    for (int i = 1; i <= N; i++)
    {
        for (int j = 1; j <= N; j++)
        {
            u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
            v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
        }
    }

    setBoundary(N, 1, u);
    setBoundary(N, 2, v);
}

void FluidSolver::setBoundary(int N, int b, float *x)
{
    /*for (int i = 1; i <= N; i++)
    {
        x[IX(0, i)] = 0.0f;
        x[IX(N + 1, i)] = 0.0f;
        x[IX(i, 0)] = 0.0f;
        x[IX(i, N + 1)] = 0.0f;
    }*/

    for (int i = 1; i <= N; i++)
    {
        x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
        x[IX(N + 1, i)] = b == 1 ? -x[IX(N, i)] : x[IX(N, i)];
        x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
    }

    x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
    x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
    x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

void FluidSolver::densStep(int N, float *x, float *x0, float *u, float *v, float diff, float dt)
{
    SWAP(x0, x);
    diffuse(N, 0, x, x0, diff, dt);
    SWAP(x0, x);
    advect(N, 0, x, x0, u, v, dt);
}

void FluidSolver::velStep(int N, float *u, float *v, float *u0, float *v0, float visc, float dt)
{
    SWAP(u0, u);
    diffuse(N, 1, u, u0, visc, dt);

    SWAP(v0, v);
    diffuse(N, 2, v, v0, visc, dt);

    project(N, u, v, u0, v0);

    SWAP(u0, u);
    SWAP(v0, v);

    advect(N, 1, u, u0, u0, v0, dt);
    advect(N, 2, v, v0, u0, v0, dt);

    project(N, u, v, u0, v0);
}

void FluidSolver::clearPrev()
{
    memset(u_prev, 0, size * sizeof(float));
    memset(v_prev, 0, size * sizeof(float));
    memset(dens_prev, 0, size * sizeof(float));
}

void FluidSolver::clear()
{
    memset(u, 0, size * sizeof(float));
    memset(v, 0, size * sizeof(float));
    memset(dens, 0, size * sizeof(float));
    clearPrev();
}

void FluidSolver::addSources(bool density)
{
    clearPrev();
    for (Source &s : sources)
    {
        int idx = IX(s.x, s.y);
        if (density && s.densitySource)
        {
            dens_prev[idx] = s.amount0;
            dens[idx] += s.amount0 * dt;
        }
        else if (!density && !s.densitySource)
        {
            u_prev[idx] = s.amount0;
            v_prev[idx] = s.amount1;
            u[idx] += s.amount0 * dt;
            v[idx] += s.amount1 * dt;
        }
    }
}

sf::Vector2f FluidSolver::densityBounds()
{
    float minVal = 0.0f;
    float maxVal = 0.0f;
    for (int i = 0; i < size; i++)
    {
        minVal = std::min(minVal, dens[i]);
        maxVal = std::max(maxVal, dens[i]);
    }
    return {minVal, maxVal};
}

void FluidSolver::update()
{
    addSources(false);
    velStep(N, u, v, u_prev, v_prev, visc, dt);
    addSources(true);
    densStep(N, dens, dens_prev, u, v, diff, dt);
    sources.clear();
}

void FluidSolver::addSource(Source source)
{
    sources.push_back(source);
}