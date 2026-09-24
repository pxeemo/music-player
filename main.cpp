#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QWheelEvent>

#include <functional>

// TEMPORARY verification harness - restored to the clean version afterwards.

// QML gives inline instances a generated name such as QQuickFlickable_QML_6,
// so match on a prefix rather than the exact class name.
static bool classMatches(QObject *o, const char *prefix)
{
    return QByteArray(o->metaObject()->className()).startsWith(prefix);
}

static QObject *findByClass(QObject *root, const char *prefix, int depth = 0)
{
    if (!root || depth > 30)
        return nullptr;
    if (classMatches(root, prefix))
        return root;
    for (QObject *c : root->children())
        if (QObject *found = findByClass(c, prefix, depth + 1))
            return found;
    if (auto *item = qobject_cast<QQuickItem *>(root))
        for (QQuickItem *child : item->childItems())
            if (QObject *found = findByClass(child, prefix, depth + 1))
                return found;
    return nullptr;
}

static QQuickItem *columnOf(QObject *scroller)
{
    auto *content = scroller->property("contentItem").value<QQuickItem *>();
    if (!content)
        return nullptr;
    for (QQuickItem *child : content->childItems()) {
        if (classMatches(child, "QQuickColumn"))
            return child;
    }
    return nullptr;
}

static void report(const char *tag, QQuickWindow *window, QObject *scroller,
                   QObject *clock)
{
    QQuickItem *column = columnOf(scroller);
    const qreal viewH = scroller->property("height").toReal();
    const qreal contentH = scroller->property("contentHeight").toReal();
    const qreal contentY = scroller->property("contentY").toReal();
    const qreal colY = column ? column->y() : 0;
    const int active = window->property("activeLine").toInt();

    QObject *animation = nullptr;
    for (QObject *c : scroller->children())
        if (classMatches(c, "QQuickNumberAnimation"))
            animation = c;

    fprintf(stderr,
            "\n%s\n        pos=%.2f active=%d contentY=%.6g contentH=%.6g "
            "viewH=%.6g colY=%.6g animRunning=%s animTo=%.6g\n",
            tag, clock->property("position").toReal(), active, contentY, contentH,
            viewH, colY,
            animation && animation->property("running").toBool() ? "yes" : "no",
            animation ? animation->property("to").toReal() : -1.0);

    if (column && active >= 0) {
        // What centerOn(active) should have converged to by now.
        for (QQuickItem *item : column->childItems()) {
            if (!item->property("index").isValid()) // skips the Repeater itself
                continue;
            if (item->property("index").toInt() != active)
                continue;
            const qreal raw = colY + item->y() + item->height() / 2 - viewH / 2;
            const qreal target =
                qBound<qreal>(0.0, raw, qMax<qreal>(0.0, contentH - viewH));
            fprintf(stderr, "        line[%d] y=%.1f h=%.1f expected=%.1f delta=%.6g\n",
                    active, item->y(), item->height(), target, contentY - target);
        }
    }
    fflush(stderr);
}

static void wheelDown(QQuickWindow *w, int ticks)
{
    const QPointF p(w->width() / 2.0, w->height() / 2.0);
    bool accepted = false;
    for (int n = 0; n < ticks; ++n) {
        QWheelEvent ev(p, p, QPoint(0, 0), QPoint(0, -120), Qt::NoButton,
                       Qt::NoModifier, Qt::NoScrollPhase, false);
        accepted = QCoreApplication::sendEvent(w, &ev);
    }
    fprintf(stderr, "        WHEEL x%d (scroll down) accepted=%s\n", ticks,
            accepted ? "true" : "false");
    fflush(stderr);
}

static void dragUp(QQuickWindow *w, qreal dy)
{
    const QPointF p(w->width() / 2.0, w->height() / 2.0);
    QMouseEvent press(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    const bool pressed = QCoreApplication::sendEvent(w, &press);
    QPointF q = p;
    for (int k = 1; k <= 10; ++k) {
        q = QPointF(p.x(), p.y() + dy * k / 10.0);
        QMouseEvent move(QEvent::MouseMove, q, q, q, Qt::NoButton, Qt::LeftButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(w, &move);
    }
    QMouseEvent release(QEvent::MouseButtonRelease, q, q, q, Qt::NoButton,
                        Qt::NoButton, Qt::NoModifier);
    const bool released = QCoreApplication::sendEvent(w, &release);
    fprintf(stderr, "        DRAG dy=%.0f pressed=%s released=%s\n", dy,
            pressed ? "true" : "false", released ? "true" : "false");
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Karaoke/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    QObject *scroller = findByClass(window, "QQuickFlickable");
    QObject *clock = findByClass(window, "PlaybackClock");
    if (!scroller || !clock) {
        fprintf(stderr, "HARNESS: scroller=%p clock=%p\n", scroller, clock);
        return EXIT_FAILURE;
    }

    enum Action { None, Wheel, Drag };
    struct Step {
        int delay;        // ms since the previous step
        const char *tag;
        Action action;
    };
    // Manual input is tested between activations (line1 @4.00, line2 @7.45) so
    // an auto-scroll cannot mask the result. Line activations: line5 @15.90,
    // line9 @31.00, line15 @58.70.
    const QList<Step> steps = {
        {  4500, "baseline (t=4.5s, after line1 auto-scroll)", None },
        {   100, "wheel x3 (scroll down)",                      Wheel },
        {   700, "settled after wheel",                         None },
        {   400, "drag up 150px",                               Drag  },
        {   700, "settled after drag",                          None },
        { 10200, "t=16.6  line5 active@15.90 (auto)",           None },
        { 15100, "t=31.7  line9 active@31.00 (auto)",           None },
        { 27700, "t=59.4  line15 active@58.70 (auto, clamped)", None },
    };

    int i = 0;
    int grab = 0;
    std::function<void()> next = [&]() {
        if (i >= steps.size()) {
            QCoreApplication::exit(0);
            return;
        }
        // Captured BY VALUE: this function returns before the timer fires, so a
        // reference to the local `step` would dangle (and did, last run).
        const Step step = steps.at(i++);
        QTimer::singleShot(step.delay, [step, window, scroller, clock, &grab, &next]() {
            switch (step.action) {
            case Wheel:
                wheelDown(window, 3);
                break;
            case Drag:
                dragUp(window, -150);
                break;
            case None:
                break;
            }
            report(step.tag, window, scroller, clock);
            const QString path = QString("/tmp/opencode/scroll%1.png").arg(++grab);
            window->grabWindow().save(path);
            fprintf(stderr, "        -> %s\n", qPrintable(path));
            fflush(stderr);
            next();
        });
    };
    next();

    return app.exec();
}
