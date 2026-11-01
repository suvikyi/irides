#pragma once

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

class HyprionGlazeManager;

// Requests a compositor-side Hyprion glaze for an item's surface-local box.
// It is inert when the Hyprion protocol is unavailable or the item is not on a
// Wayland surface; Glaze.qml then keeps its local ShaderEffect path.
class HyprionGlaze : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem* item READ item WRITE setItem NOTIFY itemChanged)
    Q_PROPERTY(qreal insetSize READ insetSize WRITE setInsetSize NOTIFY insetSizeChanged)
    Q_PROPERTY(qreal outsetSize READ outsetSize WRITE setOutsetSize NOTIFY outsetSizeChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(qreal roundingPower READ roundingPower WRITE setRoundingPower NOTIFY roundingPowerChanged)
    Q_PROPERTY(qreal lightness READ lightness WRITE setLightness NOTIFY lightnessChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(qreal chroma READ chroma WRITE setChroma NOTIFY chromaChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QColor tint READ tint WRITE setTint NOTIFY tintChanged)
    Q_PROPERTY(qreal tintStrength READ tintStrength WRITE setTintStrength NOTIFY tintStrengthChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

  public:
    void managerChanged();

    explicit HyprionGlaze(QObject* parent = nullptr);
    ~HyprionGlaze() override;

    QQuickItem* item() const;
    void        setItem(QQuickItem* item);
    qreal       insetSize() const;
    void        setInsetSize(qreal value);
    qreal       outsetSize() const;
    void        setOutsetSize(qreal value);
    qreal       radius() const;
    void        setRadius(qreal value);
    qreal       roundingPower() const;
    void        setRoundingPower(qreal value);
    qreal       lightness() const;
    void        setLightness(qreal value);
    qreal       saturation() const;
    void        setSaturation(qreal value);
    qreal       chroma() const;
    void        setChroma(qreal value);
    qreal       hue() const;
    void        setHue(qreal value);
    qreal       strength() const;
    void        setStrength(qreal value);
    qreal       opacity() const;
    void        setOpacity(qreal value);
    QColor      tint() const;
    void        setTint(const QColor& value);
    qreal       tintStrength() const;
    void        setTintStrength(qreal value);
    bool        active() const;

  signals:
    void itemChanged();
    void insetSizeChanged();
    void outsetSizeChanged();
    void radiusChanged();
    void roundingPowerChanged();
    void lightnessChanged();
    void saturationChanged();
    void chromaChanged();
    void hueChanged();
    void strengthChanged();
    void opacityChanged();
    void tintChanged();
    void tintStrengthChanged();
    void activeChanged();

  private:
    void attachWindow();
    void detachWindow();
    void attachSurface();
    void detachSurface();
    void scheduleFlush();
    void flush();
    void setActive(bool active);

    HyprionGlazeManager* m_manager = nullptr;
    QPointer<QQuickItem> m_item;
    QPointer<QQuickWindow> m_window;
    void*                m_waylandWindow = nullptr;
    void*                m_surface       = nullptr;
    void*                m_glaze         = nullptr;

    qreal  m_insetSize = 0;
    qreal  m_outsetSize = 0;
    qreal  m_radius = 0;
    qreal  m_roundingPower = 2;
    qreal  m_lightness = 0;
    qreal  m_saturation = 1;
    qreal  m_chroma = 0;
    qreal  m_hue = 0;
    qreal  m_strength = 1;
    qreal  m_opacity = 1;
    QColor m_tint = QColor(Qt::transparent);
    qreal  m_tintStrength = 0;
    bool   m_active = false;
    bool   m_flushQueued = false;
};
