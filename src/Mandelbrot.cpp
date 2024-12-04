#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <complex>
#include <iomanip>
#include <thread>

using Complex = std::complex<long double>;

namespace {
auto calculate(const Complex& c, const int iteration_limit) noexcept
{
    auto iterations = 0;
    for (auto z = Complex(); std::norm(z) <= 4 && iterations < iteration_limit; ++iterations)
        z = z * z + c;
    return iterations;
}

auto color(const int iterations, const int iteration_limit) noexcept -> sf::Color
{
    const auto hue = iterations % 360;
    const auto sat = 0.8f;
    const auto val = (iteration_limit == iterations) ? 0.f : 1.f;

    const auto h = hue / 60;
    const auto f = float(hue) / 60 - float(h);
    const auto p = val * (1 - sat);
    const auto q = val * (1 - sat * f);
    const auto t = val * (1 - sat * (1 - f));

    switch (h) {
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
    default:
        return { uint8_t(val * 255), uint8_t(t * 255), uint8_t(p * 255) };
    }
}
}

int main()
{
    constexpr auto length = std::size_t { 600 };
    constexpr auto initial_origin = Complex(-0.5, 0);
    constexpr auto initial_extent = Complex::value_type(2.5);
    constexpr auto initial_iteration_limit = 250;
    constexpr auto max_extent = 4 * initial_extent;

    auto image = sf::Image(sf::Vector2u(length, length));

    auto origin = initial_origin;
    auto extent = initial_extent;
    auto iteration_limit = initial_iteration_limit;
    auto threads = std::vector<std::thread>(std::thread::hardware_concurrency());
    auto clock = sf::Clock();
    auto recalculate = true;
    auto texture = std::optional<sf::Texture>();
    const auto font = sf::Font("data/font.ttf");

    const auto sound_buffer = sf::SoundBuffer("data/beep.wav");
    auto zoom_sound = sf::Sound(sound_buffer);
    zoom_sound.setVolume(25);

    auto text = sf::Text(font, "", 24);
    text.setFillColor(sf::Color::White);
    text.setOutlineThickness(2);
    text.setOutlineColor(sf::Color::Black);
    text.setPosition({ 10, 5 });

    const auto render_rows = [&image, &extent, &origin, &iteration_limit](const std::size_t start,
                                                                          const std::size_t end) noexcept {
        for (std::size_t i = start; i < end; ++i)
            for (std::size_t j = 0; j < length; ++j)
                image.setPixel(
                    sf::Vector2u(sf::Vector2(j, i)),
                    color(calculate(extent * Complex(double(j) / length - 0.5, -double(i) / length + 0.5) + origin,
                                    iteration_limit),
                          iteration_limit));
    };

    auto window
        = sf::RenderWindow(sf::VideoMode({ length, length }), "Mandelbrot", sf::Style::Default ^ sf::Style::Resize);
    window.setFramerateLimit(60);
    while (true) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                return 0;

            if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed>()) {
                switch (key_pressed->scancode) {
                case sf::Keyboard::Scan::Escape:
                    return 0;
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
                    zoom_sound.setPitch(zoom_sound.getPitch() * 1.02f);
                    zoom_sound.play();
                    break;
                case sf::Keyboard::Scan::S:
                    if (extent * 1.5 <= max_extent) {
                        extent = extent * 1.5;
                        zoom_sound.setPitch(zoom_sound.getPitch() / 1.02f);
                        zoom_sound.play();
                    }
                    break;
                case sf::Keyboard::Scan::R:
                    origin = initial_origin;
                    extent = initial_extent;
                    iteration_limit = initial_iteration_limit;
                    zoom_sound.setPitch(1);
                    break;
                case sf::Keyboard::Scan::RBracket:
                    iteration_limit += 25;
                    break;
                case sf::Keyboard::Scan::LBracket:
                    iteration_limit = std::max(iteration_limit - 25, 25);
                    break;
                default:
                    break;
                }
                recalculate = true;
            } else if (const auto* mouse_button_pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                origin += extent
                    * Complex(double(mouse_button_pressed->position.x) / double(window.getSize().x) - 0.5,
                              -double(mouse_button_pressed->position.y) / double(window.getSize().y) + 0.5);
                recalculate = true;
            } else if (const auto* mouse_wheel_scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (mouse_wheel_scrolled->delta > 0)
                    extent /= 1.2;
                else if (mouse_wheel_scrolled->delta < 0)
                    extent = std::min(extent * 1.2, max_extent);
                recalculate = true;
            }
        }

        window.clear();

        if (recalculate) {
            recalculate = false;

            const auto rows_per_thread = std::size_t(length / threads.size());
            for (std::size_t i = 0; i < threads.size(); ++i)
                threads[i] = std::thread(render_rows, i * rows_per_thread, (i + 1) * rows_per_thread);
            for (auto& thread : threads)
                thread.join();

            texture = sf::Texture(image);
        }

        window.draw(sf::Sprite(*texture));
        window.draw(text);
        window.display();

        auto text_builder = std::ostringstream();
        text_builder << std::setw(2) << int(1 / clock.restart().asSeconds()) << " fps\n";
        text_builder << iteration_limit << " iters\n";
        text_builder << std::setprecision(1) << std::scientific << initial_extent / extent << '\n';
        text.setString(text_builder.str());
    }
}
