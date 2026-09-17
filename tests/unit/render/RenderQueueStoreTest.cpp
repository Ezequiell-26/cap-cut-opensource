#include "render/RenderQueueStore.hpp"

#include <gtest/gtest.h>
#include <QTemporaryDir>

TEST(RenderQueueStoreTest, RoundTripsQueue) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("queue.json"));
    QVector<ccos::render::PersistedRenderJob> jobs;
    jobs.append({QStringLiteral("job-1"), QStringLiteral("out.mp4"), QStringLiteral("h264"),
                 QStringLiteral("queued"), 0.25, QString()});

    QString error;
    ASSERT_TRUE(ccos::render::RenderQueueStore::save(path, jobs, &error)) << error.toStdString();

    QVector<ccos::render::PersistedRenderJob> loaded;
    ASSERT_TRUE(ccos::render::RenderQueueStore::load(path, &loaded, &error)) << error.toStdString();
    ASSERT_EQ(loaded.size(), 1);
    EXPECT_EQ(loaded.first().id, QStringLiteral("job-1"));
    EXPECT_DOUBLE_EQ(loaded.first().progress, 0.25);
}
