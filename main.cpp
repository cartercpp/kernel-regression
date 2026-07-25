#include <iostream>
#include <format>
#include <array>
#include <utility>
#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <stop_token>
#include <cmath>

int main()
{
    constexpr int rows = 80,
                  columns = 120;

    char grid[rows][columns];

    auto resetGrid = [&grid]()
    {
        for (int row = 0; row < rows; ++row)
            for (int column = 0; column < columns; ++column)
                grid[row][column] = ' ';
    };

    constexpr double minX = 0,
                     maxX = 1,
                     xIncrement = (maxX - minX) / (columns - 1),
                     minY = -1,
                     maxY = 1,
                     bandwidth = (maxX - minX) / 20;

    std::random_device rd;
    std::uniform_real_distribution<double> yDist(minY, maxY),
                                           coeffDist(0.8, 1.2);
    std::array<std::pair<double, double>, columns> points;

    auto generatePoints = [&]()
    {
        points[0].first = minX;
        points[0].second = yDist(rd);

        for (int i = 1; i < columns; ++i)
        {
            const auto [prevX, prevY] = points[i - 1];

            points[i].first = prevX + xIncrement;
            points[i].second = std::clamp(prevY * coeffDist(rd), minY, maxY);
        }
    };

    auto moveCursor = [](int row, int column)
    {
        std::cout << std::format("\033[{};{}H", row + 1, column + 1);
    };

    auto changeColor = [](int red, int green, int blue)
    {
        std::cout << std::format("\033[38;2;{};{};{}m", red, green, blue);
    };

    std::cout << "\033[?25l";
    std::cout << "\033[2J";

    std::jthread thr{[&](std::stop_token st)
    {
        while (!st.stop_requested())
        {
            resetGrid();
            generatePoints();

            for (const auto [x1, y1] : points)
            {
                const int row = rows - 1 - static_cast<int>((y1 - minY) / (maxY - minY) * (rows - 1)),
                          column = static_cast<int>((x1 - minX) / (maxX - minX) * (columns - 1));
                if ((row >= 0) && (row < rows) && (column >= 0) && (column < columns))
                    grid[row][column] = 'o';

                double newY = 0,
                       totalWeight = 0;
                for (const auto [x2, y2] : points)
                {
                    const double distance = std::abs(x2 - x1);
                    const double weight = std::exp(-(distance * distance) / (2 * bandwidth * bandwidth));
                    newY += y2 * weight;
                    totalWeight += weight;
                }

                newY /= totalWeight;

                const int newRow = rows - 1 - static_cast<int>((newY - minY) / (maxY - minY) * (rows - 1));
                if ((newRow >= 0) && (newRow < rows) && (column >= 0) && (column < columns))
                    grid[newRow][column] = '-';
            }

            moveCursor(0, 0);
            for (int row = 0; row < rows; ++row)
            {
                for (int column = 0; column < columns; ++column)
                {
                    switch (grid[row][column])
                    {
                        case '-':
                            changeColor(0, 255, 255);
                            break;
                        case 'o':
                            changeColor(255, 0, 0);
                            break;
                    }

                    std::cout << grid[row][column];
                    std::cout << "\033[0m";
                }

                std::cout << '\n';
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }};
    std::cin.get();

    moveCursor(rows, 0);
    std::cout << "\033[?25h";
}