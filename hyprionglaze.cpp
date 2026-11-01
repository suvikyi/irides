#include "hyprionglaze.hpp"

#include "hyprion-glaze-v1-client-protocol.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QTimer>
#include <qguiapplication_platform.h>

#include <private/qwaylandwindow_p.h>

#include <algorithm>
#include <cmath>
#include <vector>

class HyprionGlazeManager {
      public:
        static HyprionGlazeManager& instance() {
            static HyprionGlazeManager manager;
            return manager;
        }

        bool                       available() const {
            return m_manager != nullptr;
        }

        wl_display*                display() const {
            return m_display;
        }

        hyprion_glaze_manager_v1* manager() const {
            return m_manager;
        }

        void add(HyprionGlaze* glaze) {
            if (std::ranges::find(m_glazes, glaze) == m_glazes.end())
                m_glazes.push_back(glaze);
        }

        void remove(HyprionGlaze* glaze) {
            std::erase(m_glazes, glaze);
        }

      private:
        HyprionGlazeManager() {
            if (!qGuiApp)
                return;
            auto* wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
            if (!wayland || !wayland->display())
                return;

            m_display = wayland->display();
            m_registry = wl_display_get_registry(m_display);
            if (!m_registry)
                return;
            wl_registry_add_listener(m_registry, &m_registryListener, this);
            wl_display_roundtrip(m_display);
        }

        static void global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
            auto& self = *static_cast<HyprionGlazeManager*>(data);
            if (self.m_manager || strcmp(interface, hyprion_glaze_manager_v1_interface.name) != 0)
                return;
            self.m_manager = static_cast<hyprion_glaze_manager_v1*>(wl_registry_bind(registry, name, &hyprion_glaze_manager_v1_interface, std::min(version, 1u)));
            if (!self.m_manager)
                return;
            for (auto* glaze : self.m_glazes)
                glaze->managerChanged();
        }

        static void globalRemove(void*, wl_registry*, uint32_t) {}

        static inline const wl_registry_listener m_registryListener = {global, globalRemove};

        wl_display*               m_display = nullptr;
        wl_registry*              m_registry = nullptr;
        hyprion_glaze_manager_v1* m_manager = nullptr;
        std::vector<HyprionGlaze*> m_glazes;
};

HyprionGlaze::HyprionGlaze(QObject* parent) : QObject(parent) {
    m_manager = &HyprionGlazeManager::instance();
    m_manager->add(this);
}

HyprionGlaze::~HyprionGlaze() {
    detachSurface();
    detachWindow();
    if (m_manager)
        m_manager->remove(this);
}

QQuickItem* HyprionGlaze::item() const {
    return m_item;
}

void HyprionGlaze::setItem(QQuickItem* item) {
    if (m_item == item)
        return;
    detachSurface();
    detachWindow();
    if (m_item)
        QObject::disconnect(m_item, nullptr, this, nullptr);
    m_item = item;
    if (m_item) {
        connect(m_item, &QQuickItem::xChanged, this, &HyprionGlaze::scheduleFlush);
        connect(m_item, &QQuickItem::yChanged, this, &HyprionGlaze::scheduleFlush);
        connect(m_item, &QQuickItem::widthChanged, this, &HyprionGlaze::scheduleFlush);
        connect(m_item, &QQuickItem::heightChanged, this, &HyprionGlaze::scheduleFlush);
        connect(m_item, &QQuickItem::visibleChanged, this, &HyprionGlaze::scheduleFlush);
        connect(m_item, &QQuickItem::windowChanged, this, &HyprionGlaze::attachWindow);
        connect(m_item, &QObject::destroyed, this, [this] { detachWindow(); m_item = nullptr; });
        attachWindow();
    }
    emit itemChanged();
}

qreal HyprionGlaze::insetSize() const {
    return m_insetSize;
}
void HyprionGlaze::setInsetSize(qreal value) {
    if (m_insetSize == value)
        return;
    m_insetSize = value;
    emit insetSizeChanged();
    scheduleFlush();
}
qreal HyprionGlaze::outsetSize() const {
    return m_outsetSize;
}
void HyprionGlaze::setOutsetSize(qreal value) {
    if (m_outsetSize == value)
        return;
    m_outsetSize = value;
    emit outsetSizeChanged();
    // v1 draws the inset band only. Keep the local ShaderEffect for the
    // outset rather than silently dropping it.
    if (m_outsetSize > 0)
        detachSurface();
    else if (m_manager && m_manager->available())
        attachSurface();
    scheduleFlush();
}
qreal HyprionGlaze::radius() const {
    return m_radius;
}
void HyprionGlaze::setRadius(qreal value) {
    if (m_radius == value)
        return;
    m_radius = value;
    emit radiusChanged();
    scheduleFlush();
}
qreal HyprionGlaze::roundingPower() const {
    return m_roundingPower;
}
void HyprionGlaze::setRoundingPower(qreal value) {
    if (m_roundingPower == value)
        return;
    m_roundingPower = value;
    emit roundingPowerChanged();
    scheduleFlush();
}
qreal HyprionGlaze::lightness() const {
    return m_lightness;
}
void HyprionGlaze::setLightness(qreal value) {
    if (m_lightness == value)
        return;
    m_lightness = value;
    emit lightnessChanged();
    scheduleFlush();
}
qreal HyprionGlaze::saturation() const {
    return m_saturation;
}
void HyprionGlaze::setSaturation(qreal value) {
    if (m_saturation == value)
        return;
    m_saturation = value;
    emit saturationChanged();
    scheduleFlush();
}
qreal HyprionGlaze::chroma() const {
    return m_chroma;
}
void HyprionGlaze::setChroma(qreal value) {
    if (m_chroma == value)
        return;
    m_chroma = value;
    emit chromaChanged();
    scheduleFlush();
}
qreal HyprionGlaze::hue() const {
    return m_hue;
}
void HyprionGlaze::setHue(qreal value) {
    if (m_hue == value)
        return;
    m_hue = value;
    emit hueChanged();
    scheduleFlush();
}
qreal HyprionGlaze::strength() const {
    return m_strength;
}
void HyprionGlaze::setStrength(qreal value) {
    if (m_strength == value)
        return;
    m_strength = value;
    emit strengthChanged();
    scheduleFlush();
}
qreal HyprionGlaze::opacity() const {
    return m_opacity;
}
void HyprionGlaze::setOpacity(qreal value) {
    if (m_opacity == value)
        return;
    m_opacity = value;
    emit opacityChanged();
    scheduleFlush();
}
QColor HyprionGlaze::tint() const {
    return m_tint;
}
void HyprionGlaze::setTint(const QColor& value) {
    if (m_tint == value)
        return;
    m_tint = value;
    emit tintChanged();
    scheduleFlush();
}
qreal HyprionGlaze::tintStrength() const {
    return m_tintStrength;
}
void HyprionGlaze::setTintStrength(qreal value) {
    if (m_tintStrength == value)
        return;
    m_tintStrength = value;
    emit tintStrengthChanged();
    scheduleFlush();
}
bool HyprionGlaze::active() const {
    return m_active;
}

void HyprionGlaze::managerChanged() {
    if (!m_manager || !m_manager->available()) {
        detachSurface();
        setActive(false);
        return;
    }
    attachSurface();
}

void HyprionGlaze::attachWindow() {
    if (!m_item)
        return;

    auto* itemWindow = m_item->window();
    if (m_window != itemWindow) {
        if (m_window)
            QObject::disconnect(m_window, nullptr, this, nullptr);
        m_window = itemWindow;
        if (m_window) {
            connect(m_window, &QWindow::visibleChanged, this, [this](bool) { attachWindow(); });
            connect(m_window, &QQuickWindow::sceneGraphInitialized, this, &HyprionGlaze::attachWindow);
        }
    }

    if (!m_window || m_waylandWindow)
        return;

    auto* window = dynamic_cast<QtWaylandClient::QWaylandWindow*>(m_window->handle());
    if (!window)
        return;

    m_waylandWindow = window;
    connect(window, &QtWaylandClient::QWaylandWindow::surfaceCreated, this, &HyprionGlaze::attachSurface);
    connect(window, &QtWaylandClient::QWaylandWindow::surfaceDestroyed, this, &HyprionGlaze::detachSurface);
    connect(window, &QObject::destroyed, this, &HyprionGlaze::detachWindow);
    if (window->surface())
        attachSurface();
}

void HyprionGlaze::detachWindow() {
    if (m_window)
        QObject::disconnect(m_window, nullptr, this, nullptr);
    if (m_waylandWindow) {
        auto* window = static_cast<QtWaylandClient::QWaylandWindow*>(m_waylandWindow);
        QObject::disconnect(window, nullptr, this, nullptr);
        m_waylandWindow = nullptr;
    }
    m_window = nullptr;
    detachSurface();
}

void HyprionGlaze::attachSurface() {
    if (!m_waylandWindow || !m_manager || !m_manager->available() || m_outsetSize > 0)
        return;
    auto* window = static_cast<QtWaylandClient::QWaylandWindow*>(m_waylandWindow);
    if (!window || !window->surface() || m_surface == window->surface())
        return;

    detachSurface();
    m_surface = window->surface();
    m_glaze = hyprion_glaze_manager_v1_get_glaze(m_manager->manager(), static_cast<wl_surface*>(m_surface));
    setActive(m_glaze != nullptr);
    scheduleFlush();
}

void HyprionGlaze::detachSurface() {
    if (m_glaze) {
        hyprion_glaze_v1_destroy(static_cast<hyprion_glaze_v1*>(m_glaze));
        m_glaze = nullptr;
    }
    m_surface = nullptr;
    setActive(false);
}

void HyprionGlaze::setActive(bool active) {
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
}

void HyprionGlaze::scheduleFlush() {
    if (m_flushQueued)
        return;
    m_flushQueued = true;
    QTimer::singleShot(0, this, [this] {
        m_flushQueued = false;
        flush();
    });
}

void HyprionGlaze::flush() {
    if (!m_item || !m_window || !m_glaze || !m_manager || !m_manager->display() || m_outsetSize > 0)
        return;

    const QPointF position = m_item->mapToItem(m_window->contentItem(), QPointF(0, 0));
    const qreal   outset = std::max<qreal>(m_outsetSize, 0);
    const int     x = qRound(position.x() + outset);
    const int     y = qRound(position.y() + outset);
    const int     width = qRound(m_item->width() - 2 * outset);
    const int     height = qRound(m_item->height() - 2 * outset);
    if (width <= 0 || height <= 0)
        return;

    auto* glaze = static_cast<hyprion_glaze_v1*>(m_glaze);
    hyprion_glaze_v1_set_box(glaze, x, y, width, height, wl_fixed_from_double(m_radius), wl_fixed_from_double(m_roundingPower));
    hyprion_glaze_v1_set_bands(glaze, qRound(m_insetSize), qRound(m_outsetSize));

    const QColor tint = m_tint;
    const uint32_t tintArgb = (static_cast<uint32_t>(tint.red()) << 16) | (static_cast<uint32_t>(tint.green()) << 8) | static_cast<uint32_t>(tint.blue());
    hyprion_glaze_v1_set_style(glaze, wl_fixed_from_double(m_lightness), wl_fixed_from_double(m_saturation), wl_fixed_from_double(m_chroma),
                               wl_fixed_from_double(m_hue), wl_fixed_from_double(m_strength), wl_fixed_from_double(m_opacity), tintArgb,
                               wl_fixed_from_double(m_tintStrength));
    hyprion_glaze_v1_set_source(glaze, HYPRION_GLAZE_V1_SOURCE_INSET_FILL);
    wl_display_flush(m_manager->display());
}
