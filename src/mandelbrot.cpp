#include <SFML/Graphics.hpp>

#include <array>
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
#include <thread>

using Complex = std::complex<double>;

static int g_max_iterations { 250 };

static auto Calculate(const Complex& c)
{
    auto iterations = 0;
    for (auto z = Complex(); std::abs(z) <= 2 && iterations < g_max_iterations; ++iterations)
        z = z * z + c;
    return iterations;
}

static auto Color(const int iterations)
{
    const auto hue = iterations % 360;
    const auto sat = 1.0f;
    const auto val = (g_max_iterations == iterations) ? 0.0f : 1.0f;

    const auto h = hue / 60;
    const auto f = (float)hue / 60 - h;
    const auto p = val * (1.0f - sat);
    const auto q = val * (1.0f - sat * f);
    const auto t = val * (1.0f - sat * (1.0f - f));

    switch (h) {
    default:
    case 0:
    case 6:
        return sf::Color(val * 255, t * 255, p * 255);
    case 1:
        return sf::Color(q * 255, val * 255, p * 255);
    case 2:
        return sf::Color(p * 255, val * 255, t * 255);
    case 3:
        return sf::Color(p * 255, q * 255, val * 255);
    case 4:
        return sf::Color(t * 255, p * 255, val * 255);
    case 5:
        return sf::Color(val * 255, p * 255, q * 255);
    }
    throw std::runtime_error("Unreachable");
}

int main()
try {
    constexpr auto length = 800u;
    constexpr auto initial_origin = Complex(-0.5, 0.0);
    constexpr auto initial_extent = 2.5;

    // Heap allocate to accomodate systems with small (<1MB) stack sizes
    auto pixels = std::make_unique<std::array<std::array<sf::Color, length>, length>>();

    auto origin = initial_origin;
    auto extent = initial_extent;
    auto threads = std::vector<std::thread>(std::thread::hardware_concurrency());
    auto then = std::chrono::steady_clock::now();
    auto recalculate = true;
    auto font = sf::Font();
    if (!font.loadFromFile(std::string(FONT_PATH) + "/font.ttf"))
        throw std::runtime_error("Failed to load font");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);

    const auto render_rows = [&pixels, &extent, &origin](const unsigned start, const unsigned end) {
        for (unsigned i = start; i < end; ++i)
            for (unsigned j = 0; j < length; ++j)
                (*pixels)[i][j]
                    = Color(Calculate(extent * Complex((double)j / length - 0.5, -(double)i / length + 0.5) + origin));
    };

    auto window = sf::RenderWindow(sf::VideoMode(length, length), "Mandelbrot");
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        auto event = sf::Event();
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                case sf::Keyboard::Up:
                    origin = { origin.real(), origin.imag() + extent / 25.0 };
                    break;
                case sf::Keyboard::Down:
                    origin = { origin.real(), origin.imag() - extent / 25.0 };
                    break;
                case sf::Keyboard::Left:
                    origin = { origin.real() - extent / 25.0, origin.imag() };
                    break;
                case sf::Keyboard::Right:
                    origin = { origin.real() + extent / 25.0, origin.imag() };
                    break;
                case sf::Keyboard::W:
                    extent /= 1.5;
                    break;
                case sf::Keyboard::S:
                    extent *= 1.5;
                    break;
                case sf::Keyboard::R:
                    origin = initial_origin;
                    extent = initial_extent;
                    break;
                case sf::Keyboard::RBracket:
                    g_max_iterations += 25;
                    break;
                case sf::Keyboard::LBracket:
                    g_max_iterations -= 25;
                    break;
                default:
                    break;
                }
                recalculate = true;
                break;
            case sf::Event::MouseButtonPressed:
                origin = origin
                    + extent / length
                        * Complex(event.mouseButton.x - length / 2.0, -event.mouseButton.y + length / 2.0);
                recalculate = true;
                break;
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.delta > 0.0f)
                    extent /= 1.2;
                else if (event.mouseWheelScroll.delta < 0.0f)
                    extent *= 1.2;
                recalculate = true;
                break;
            default:
                break;
            }
        }

        window.clear();

        if (recalculate) {
            recalculate = false;

            for (size_t i = 0; i < threads.size(); ++i)
                threads[i] = std::thread(render_rows, i * length / threads.size(), (i + 1) * length / threads.size());
            for (auto& thread : threads)
                thread.join();
        }

        auto image = sf::Image();
        image.create(length, length, (sf::Uint8*)pixels->data());
        auto texture = sf::Texture();
        texture.loadFromImage(image);

        window.draw(sf::Sprite(texture));
        window.draw(text);
        window.display();

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - then);
        const auto framerate = 1'000'000 / elapsed.count();
        then = now;

        auto text_builder = std::ostringstream();
        text_builder << std::setw(4) << framerate << " fps\n";
        text_builder << std::setw(4) << g_max_iterations << " iters\n";
        text.setString(text_builder.str());
    }
} catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return -1;
}
