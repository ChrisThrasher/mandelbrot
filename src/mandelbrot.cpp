#include <SFML/Graphics.hpp>

#include <array>
#include <complex>
#include <iomanip>
#include <thread>

using Complex = std::complex<double>;

static constexpr int initial_max_iterations { 250 };
static int max_iterations { initial_max_iterations };

static auto Calculate(const Complex& c) noexcept
{
    auto iterations = 0;
    for (auto z = Complex(); std::norm(z) <= 4.0 && iterations < max_iterations; ++iterations)
        z = z * z + c;
    return iterations;
}

static auto Color(const int iterations) noexcept -> sf::Color
{
    const auto hue = iterations % 360;
    const auto sat = 1.0f;
    const auto val = (max_iterations == iterations) ? 0.0f : 1.0f;

    const auto h = hue / 60;
    const auto f = (float)hue / 60.0f - (float)h;
    const auto p = val * (1.0f - sat);
    const auto q = val * (1.0f - sat * f);
    const auto t = val * (1.0f - sat * (1.0f - f));

    switch (h) {
    default:
    case 0:
    case 6:
        return { sf::Uint8(val * 255), sf::Uint8(t * 255), sf::Uint8(p * 255) };
    case 1:
        return { sf::Uint8(q * 255), sf::Uint8(val * 255), sf::Uint8(p * 255) };
    case 2:
        return { sf::Uint8(p * 255), sf::Uint8(val * 255), sf::Uint8(t * 255) };
    case 3:
        return { sf::Uint8(p * 255), sf::Uint8(q * 255), sf::Uint8(val * 255) };
    case 4:
        return { sf::Uint8(t * 255), sf::Uint8(p * 255), sf::Uint8(val * 255) };
    case 5:
        return { sf::Uint8(val * 255), sf::Uint8(p * 255), sf::Uint8(0 * 255) };
    }
}

int main()
{
    constexpr auto length = 800u;
    constexpr auto initial_origin = Complex(-0.5, 0.0);
    constexpr auto initial_extent = Complex::value_type(2.5);

    // Heap allocate to accomodate systems with small (<1MB) stack sizes
    auto pixels = std::make_unique<std::array<std::array<sf::Color, length>, length>>();

    auto origin = initial_origin;
    auto extent = initial_extent;
    auto threads = std::vector<std::thread>(std::thread::hardware_concurrency());
    auto clock = sf::Clock();
    auto recalculate = true;
    auto image = sf::Image();
    auto texture = sf::Texture();
    auto font = sf::Font();
    font.loadFromFile(std::string(FONT_PATH) + "/font.ttf");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10.0f, 5.0f });

    const auto render_rows = [&pixels, &extent, &origin](const unsigned start, const unsigned end) noexcept {
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
                    max_iterations = initial_max_iterations;
                    break;
                case sf::Keyboard::RBracket:
                    max_iterations += 25;
                    break;
                case sf::Keyboard::LBracket:
                    max_iterations = std::max(max_iterations - 25, 25);
                    break;
                default:
                    break;
                }
                recalculate = true;
                break;
            case sf::Event::MouseButtonPressed:
                origin += extent / length
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

            image.create(length, length, (sf::Uint8*)pixels->data());
            texture.loadFromImage(image);
        }

        window.draw(sf::Sprite(texture));
        window.draw(text);
        window.display();

        const auto framerate = 1'000'000 / clock.getElapsedTime().asMicroseconds();
        clock.restart();

        auto text_builder = std::ostringstream();
        text_builder << std::setw(4) << framerate << " fps\n";
        text_builder << std::setw(4) << max_iterations << " iters\n";
        text_builder << std::setprecision(1) << std::scientific << initial_extent / extent << '\n';
        text.setString(text_builder.str());
    }
}
