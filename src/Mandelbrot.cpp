#include <SFML/Graphics.hpp>

#include <array>
#include <complex>
#include <future>
#include <iomanip>

using Complex = std::complex<long double>;

static constexpr auto initial_max_iterations { 250 };
static auto max_iterations { initial_max_iterations };

static auto calculate(const Complex& c) noexcept
{
    auto iterations = 0;
    for (auto z = Complex(); std::norm(z) <= 4 && iterations < max_iterations; ++iterations)
        z = z * z + c;
    return iterations;
}

static auto color(const int iterations) noexcept -> sf::Color
{
    const auto hue = iterations % 360;
    const auto sat = 0.8f;
    const auto val = (max_iterations == iterations) ? 0.f : 1.f;

    const auto h = hue / 60;
    const auto f = float(hue) / 60 - float(h);
    const auto p = val * (1 - sat);
    const auto q = val * (1 - sat * f);
    const auto t = val * (1 - sat * (1 - f));

    switch (h) {
    default:
    case 0:
    case 6:
        return { uint8_t(val * 255), uint8_t(t * 255), uint8_t(p * 255) };
    case 1:
        return { uint8_t(q * 255), uint8_t(val * 255), uint8_t(p * 255) };
    case 2:
        return { uint8_t(p * 255), uint8_t(val * 255), uint8_t(t * 255) };
    case 3:
        return { uint8_t(p * 255), uint8_t(q * 255), uint8_t(val * 255) };
    case 4:
        return { uint8_t(t * 255), uint8_t(p * 255), uint8_t(val * 255) };
    case 5:
        return { uint8_t(val * 255), uint8_t(p * 255), uint8_t(0 * 255) };
    }
}

int main()
{
    constexpr auto length = size_t(600);
    constexpr auto initial_origin = Complex(-0.5, 0);
    constexpr auto initial_extent = Complex::value_type(2.5);

    auto image = sf::Image();
    image.create({ length, length });

    auto origin = initial_origin;
    auto extent = initial_extent;
    auto futures = std::vector<std::future<void>>(std::thread::hardware_concurrency());
    auto clock = sf::Clock();
    auto recalculate = true;
    auto texture = sf::Texture();
    auto font = sf::Font();
    if (!font.loadFromFile(FONT_PATH / std::filesystem::path("font.ttf")))
        throw std::runtime_error("Failed to load font");

    auto text = sf::Text("", font, 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10, 5 });

    const auto render_rows = [&image, &extent, &origin](const unsigned start, const unsigned end) noexcept {
        for (unsigned i = start; i < end; ++i)
            for (unsigned j = 0; j < length; ++j)
                image.setPixel(
                    { j, i },
                    color(calculate(extent * Complex(double(j) / length - 0.5, -double(i) / length + 0.5) + origin)));
    };

    auto window = sf::RenderWindow(sf::VideoMode({ length, length }), "Mandelbrot");
    window.setFramerateLimit(60);
    while (window.isOpen()) {
        for (auto event = sf::Event(); window.pollEvent(event);) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.scancode) {
                case sf::Keyboard::Scan::Escape:
                    window.close();
                    break;
                case sf::Keyboard::Scan::Up:
                    origin = { origin.real(), origin.imag() + extent / 25 };
                    break;
                case sf::Keyboard::Scan::Down:
                    origin = { origin.real(), origin.imag() - extent / 25 };
                    break;
                case sf::Keyboard::Scan::Left:
                    origin = { origin.real() - extent / 25, origin.imag() };
                    break;
                case sf::Keyboard::Scan::Right:
                    origin = { origin.real() + extent / 25, origin.imag() };
                    break;
                case sf::Keyboard::Scan::W:
                    extent /= 1.5;
                    break;
                case sf::Keyboard::Scan::S:
                    extent *= 1.5;
                    break;
                case sf::Keyboard::Scan::R:
                    origin = initial_origin;
                    extent = initial_extent;
                    max_iterations = initial_max_iterations;
                    break;
                case sf::Keyboard::Scan::RBracket:
                    max_iterations += 25;
                    break;
                case sf::Keyboard::Scan::LBracket:
                    max_iterations = std::max(max_iterations - 25, 25);
                    break;
                default:
                    break;
                }
                recalculate = true;
                break;
            case sf::Event::MouseButtonPressed:
                origin += extent
                    * Complex(double(event.mouseButton.x) / double(window.getSize().x) - 0.5,
                              -double(event.mouseButton.y) / double(window.getSize().y) + 0.5);
                recalculate = true;
                break;
            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.delta > 0)
                    extent /= 1.2;
                else if (event.mouseWheelScroll.delta < 0)
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

            const auto rows_per_thread = unsigned(length / futures.size());
            for (unsigned i = 0; i < futures.size(); ++i)
                futures[i]
                    = std::async(std::launch::async, render_rows, i * rows_per_thread, (i + 1) * rows_per_thread);
            for (auto& future : futures)
                future.wait();

            if (!texture.loadFromImage(image))
                throw std::runtime_error("Failed to load texture");
        }

        window.draw(sf::Sprite(texture));
        window.draw(text);
        window.display();

        auto text_builder = std::ostringstream();
        text_builder << std::setw(4) << int(1 / clock.restart().asSeconds()) << " fps\n";
        text_builder << std::setw(4) << max_iterations << " iters\n";
        text_builder << std::setprecision(1) << std::scientific << initial_extent / extent << '\n';
        text.setString(text_builder.str());
    }
}
