#include "hypriongrade.hpp"

#include "hyprion-grade-v1-client-protocol.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QTimer>
#include <qguiapplication_platform.h>

#include <private/qwaylandwindow_p.h>

#include <algorithm>
#include <cmath>
#include <vector>

class HyprionGradeManager {
      public:
        static HyprionGradeManager& instance() {
            static HyprionGradeManager manager;
            return manager;
        }

        bool                       available() const {
            return m_manager != nullptr;
        }

        bool                       supports(uint32_t areas, uint32_t sources) const {
            return m_manager && m_capabilitiesKnown && (m_areas & areas) != 0 && (m_sources & sources) != 0;
        }

        wl_display*                display() const {
            return m_display;
        }

        hyprion_grade_manager_v1* manager() const {
            return m_manager;
        }

        void add(HyprionGrade* grade) {
            if (std::ranges::find(m_grades, grade) == m_grades.end())
                m_grades.push_back(grade);
        }

        void remove(HyprionGrade* grade) {
            std::erase(m_grades, grade);
        }

      private:
        HyprionGradeManager() {
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
            auto& self = *static_cast<HyprionGradeManager*>(data);
            if (self.m_manager || strcmp(interface, hyprion_grade_manager_v1_interface.name) != 0)
                return;
            self.m_manager = static_cast<hyprion_grade_manager_v1*>(wl_registry_bind(registry, name, &hyprion_grade_manager_v1_interface, std::min(version, 1u)));
            if (!self.m_manager)
                return;
            self.m_managerName = name;
            hyprion_grade_manager_v1_add_listener(self.m_manager, &m_managerListener, &self);
            for (auto* grade : self.m_grades)
                grade->managerChanged();
        }

        static void capabilities(void* data, hyprion_grade_manager_v1*, uint32_t areas, uint32_t sources) {
            auto& self = *static_cast<HyprionGradeManager*>(data);
            self.m_areas = areas;
            self.m_sources = sources;
            self.m_capabilitiesKnown = true;
            for (auto* grade : self.m_grades)
                grade->managerChanged();
        }

        static void globalRemove(void* data, wl_registry*, uint32_t name) {
            auto& self = *static_cast<HyprionGradeManager*>(data);
            if (!self.m_manager || self.m_managerName != name)
                return;
            self.m_manager = nullptr;
            self.m_managerName = 0;
            self.m_capabilitiesKnown = false;
            self.m_areas = 0;
            self.m_sources = 0;
            for (auto* grade : self.m_grades)
                grade->managerChanged();
        }

        static inline const hyprion_grade_manager_v1_listener m_managerListener = {capabilities};

        static inline const wl_registry_listener m_registryListener = {global, globalRemove};

        wl_display*               m_display = nullptr;
        wl_registry*              m_registry = nullptr;
        hyprion_grade_manager_v1* m_manager = nullptr;
        uint32_t                  m_managerName = 0;
        bool                      m_capabilitiesKnown = false;
        uint32_t                  m_areas = 0;
        uint32_t                  m_sources = 0;
        std::vector<HyprionGrade*> m_grades;
};

namespace {
    void gradeSupported(void* data, hyprion_grade_v1*, uint32_t supported) {
        static_cast<HyprionGrade*>(data)->setSupported(supported != 0);
    }

    inline const hyprion_grade_v1_listener m_gradeListener = {gradeSupported};
}

HyprionGrade::HyprionGrade(QObject* parent) : QObject(parent) {
    m_manager = &HyprionGradeManager::instance();
    m_manager->add(this);
}

HyprionGrade::~HyprionGrade() {
    detachSurface();
    detachWindow();
    if (m_manager)
        m_manager->remove(this);
}

QQuickItem* HyprionGrade::item() const {
    return m_item;
}

void HyprionGrade::setItem(QQuickItem* item) {
    if (m_item == item)
        return;
    detachSurface();
    detachWindow();
    if (m_item)
        QObject::disconnect(m_item, nullptr, this, nullptr);
    m_item = item;
    if (m_item) {
        connect(m_item, &QQuickItem::xChanged, this, &HyprionGrade::scheduleFlush);
        connect(m_item, &QQuickItem::yChanged, this, &HyprionGrade::scheduleFlush);
        connect(m_item, &QQuickItem::widthChanged, this, &HyprionGrade::scheduleFlush);
        connect(m_item, &QQuickItem::heightChanged, this, &HyprionGrade::scheduleFlush);
        connect(m_item, &QQuickItem::visibleChanged, this, &HyprionGrade::scheduleFlush);
        connect(m_item, &QQuickItem::windowChanged, this, &HyprionGrade::attachWindow);
        connect(m_item, &QObject::destroyed, this, [this] { detachWindow(); m_item = nullptr; });
        attachWindow();
    }
    emit itemChanged();
}

int HyprionGrade::area() const {
    return m_area;
}
void HyprionGrade::setArea(int value) {
    if (m_area == value)
        return;
    m_area = value;
    emit areaChanged();
    updateActive();
    scheduleFlush();
}
int HyprionGrade::sourceMode() const {
    return m_sourceMode;
}
void HyprionGrade::setSourceMode(int value) {
    if (m_sourceMode == value)
        return;
    m_sourceMode = value;
    emit sourceModeChanged();
    updateActive();
    scheduleFlush();
}
bool HyprionGrade::enabled() const {
    return m_enabled;
}
void HyprionGrade::setEnabled(bool value) {
    if (m_enabled == value)
        return;
    m_enabled = value;
    emit enabledChanged();
    updateActive();
    scheduleFlush();
}
qreal HyprionGrade::insetSize() const {
    return m_insetSize;
}
void HyprionGrade::setInsetSize(qreal value) {
    if (m_insetSize == value)
        return;
    m_insetSize = value;
    emit insetSizeChanged();
    scheduleFlush();
}
qreal HyprionGrade::outsetSize() const {
    return m_outsetSize;
}
void HyprionGrade::setOutsetSize(qreal value) {
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
    updateActive();
    scheduleFlush();
}
qreal HyprionGrade::radius() const {
    return m_radius;
}
void HyprionGrade::setRadius(qreal value) {
    if (m_radius == value)
        return;
    m_radius = value;
    emit radiusChanged();
    scheduleFlush();
}
qreal HyprionGrade::roundingPower() const {
    return m_roundingPower;
}
void HyprionGrade::setRoundingPower(qreal value) {
    if (m_roundingPower == value)
        return;
    m_roundingPower = value;
    emit roundingPowerChanged();
    scheduleFlush();
}
qreal HyprionGrade::lightness() const {
    return m_lightness;
}
void HyprionGrade::setLightness(qreal value) {
    if (m_lightness == value)
        return;
    m_lightness = value;
    emit lightnessChanged();
    scheduleFlush();
}
qreal HyprionGrade::saturation() const {
    return m_saturation;
}
void HyprionGrade::setSaturation(qreal value) {
    if (m_saturation == value)
        return;
    m_saturation = value;
    emit saturationChanged();
    scheduleFlush();
}
qreal HyprionGrade::chroma() const {
    return m_chroma;
}
void HyprionGrade::setChroma(qreal value) {
    if (m_chroma == value)
        return;
    m_chroma = value;
    emit chromaChanged();
    scheduleFlush();
}
qreal HyprionGrade::hue() const {
    return m_hue;
}
void HyprionGrade::setHue(qreal value) {
    if (m_hue == value)
        return;
    m_hue = value;
    emit hueChanged();
    scheduleFlush();
}
qreal HyprionGrade::strength() const {
    return m_strength;
}
void HyprionGrade::setStrength(qreal value) {
    if (m_strength == value)
        return;
    m_strength = value;
    emit strengthChanged();
    scheduleFlush();
}
qreal HyprionGrade::opacity() const {
    return m_opacity;
}
void HyprionGrade::setOpacity(qreal value) {
    if (m_opacity == value)
        return;
    m_opacity = value;
    emit opacityChanged();
    scheduleFlush();
}
QColor HyprionGrade::tint() const {
    return m_tint;
}
void HyprionGrade::setTint(const QColor& value) {
    if (m_tint == value)
        return;
    m_tint = value;
    emit tintChanged();
    scheduleFlush();
}
qreal HyprionGrade::tintStrength() const {
    return m_tintStrength;
}
void HyprionGrade::setTintStrength(qreal value) {
    if (m_tintStrength == value)
        return;
    m_tintStrength = value;
    emit tintStrengthChanged();
    scheduleFlush();
}
bool HyprionGrade::active() const {
    return m_active;
}

void HyprionGrade::managerChanged() {
    if (!m_manager || !m_manager->available() || m_outsetSize > 0) {
        detachSurface();
        updateActive();
        return;
    }
    attachSurface();
    updateActive();
}

void HyprionGrade::setSupported(bool supported) {
    if (m_supported == supported)
        return;
    m_supported = supported;
    updateActive();
}

void HyprionGrade::attachWindow() {
    if (!m_item)
        return;

    auto* itemWindow = m_item->window();
    if (m_window != itemWindow) {
        if (m_window)
            QObject::disconnect(m_window, nullptr, this, nullptr);
        m_window = itemWindow;
        if (m_window) {
            connect(m_window, &QWindow::visibleChanged, this, [this](bool) { attachWindow(); });
            connect(m_window, &QQuickWindow::sceneGraphInitialized, this, &HyprionGrade::attachWindow);
        }
    }

    if (!m_window || m_waylandWindow)
        return;

    auto* window = dynamic_cast<QtWaylandClient::QWaylandWindow*>(m_window->handle());
    if (!window)
        return;

    m_waylandWindow = window;
    connect(window, &QtWaylandClient::QWaylandWindow::surfaceCreated, this, &HyprionGrade::attachSurface);
    connect(window, &QtWaylandClient::QWaylandWindow::surfaceDestroyed, this, &HyprionGrade::detachSurface);
    connect(window, &QObject::destroyed, this, &HyprionGrade::detachWindow);
    if (window->surface())
        attachSurface();
}

void HyprionGrade::detachWindow() {
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

void HyprionGrade::attachSurface() {
    if (!m_waylandWindow || !m_manager || !m_manager->available() || m_outsetSize > 0)
        return;
    auto* window = static_cast<QtWaylandClient::QWaylandWindow*>(m_waylandWindow);
    if (!window || !window->surface() || m_surface == window->surface())
        return;

    detachSurface();
    m_surface = window->surface();
    m_grade = hyprion_grade_manager_v1_get_grade(m_manager->manager(), static_cast<wl_surface*>(m_surface));
    if (m_grade)
        hyprion_grade_v1_add_listener(static_cast<hyprion_grade_v1*>(m_grade), &m_gradeListener, this);
    m_supported = false;
    updateActive();
    scheduleFlush();
}

void HyprionGrade::detachSurface() {
    if (m_grade) {
        hyprion_grade_v1_destroy(static_cast<hyprion_grade_v1*>(m_grade));
        m_grade = nullptr;
    }
    m_surface = nullptr;
    m_supported = false;
    setActive(false);
}

void HyprionGrade::updateActive() {
    const bool active = m_enabled && m_supported && m_grade && m_manager && m_manager->supports(m_area, m_sourceMode) && m_outsetSize <= 0;
    setActive(active);
}

void HyprionGrade::setActive(bool active) {
    if (m_active == active)
        return;
    m_active = active;
    emit activeChanged();
}

void HyprionGrade::scheduleFlush() {
    if (m_flushQueued)
        return;
    m_flushQueued = true;
    QTimer::singleShot(0, this, [this] {
        m_flushQueued = false;
        flush();
    });
}

void HyprionGrade::flush() {
    if (!m_item || !m_window || !m_grade || !m_manager || !m_manager->display() || m_outsetSize > 0 || !m_manager->supports(m_area, m_sourceMode))
        return;

    const QPointF position = m_item->mapToItem(m_window->contentItem(), QPointF(0, 0));
    const qreal   outset = std::max<qreal>(m_outsetSize, 0);
    const int     x = qRound(position.x() + outset);
    const int     y = qRound(position.y() + outset);
    const int     width = qRound(m_item->width() - 2 * outset);
    const int     height = qRound(m_item->height() - 2 * outset);
    if (width <= 0 || height <= 0)
        return;

    auto* grade = static_cast<hyprion_grade_v1*>(m_grade);
    hyprion_grade_v1_set_enabled(grade, m_enabled ? 1 : 0);
    hyprion_grade_v1_set_box(grade, x, y, width, height, wl_fixed_from_double(m_radius), wl_fixed_from_double(m_roundingPower));
    hyprion_grade_v1_set_area(grade, static_cast<uint32_t>(m_area), qRound(m_insetSize), qRound(m_outsetSize));

    const QColor tint = m_tint;
    const uint32_t tintArgb = (static_cast<uint32_t>(tint.red()) << 16) | (static_cast<uint32_t>(tint.green()) << 8) | static_cast<uint32_t>(tint.blue());
    hyprion_grade_v1_set_style(grade, wl_fixed_from_double(m_lightness), wl_fixed_from_double(m_saturation), wl_fixed_from_double(m_chroma),
                               wl_fixed_from_double(m_hue), wl_fixed_from_double(m_strength), wl_fixed_from_double(m_opacity), tintArgb,
                               wl_fixed_from_double(m_tintStrength));
    hyprion_grade_v1_set_source(grade, static_cast<uint32_t>(m_sourceMode));
    wl_display_flush(m_manager->display());
}
