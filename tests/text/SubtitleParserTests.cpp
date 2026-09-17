#include "text/SubtitleParser.hpp"
#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

TEST(SubtitleParserTests, ParsesSrtAndWritesIt) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString input = dir.filePath(QStringLiteral("input.srt"));
    QFile f(input);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "1\n00:00:01,000 --> 00:00:02,500\nHello\n\n";
    f.close();

    QVector<ccos::text::SubtitleCue> cues;
    QString error;
    ASSERT_TRUE(ccos::text::SubtitleParser::parseSrt(input, cues, &error)) << error.toStdString();
    ASSERT_EQ(cues.size(), 1);
    EXPECT_EQ(cues.front().text, QStringLiteral("Hello"));
    EXPECT_NEAR(cues.front().start.seconds(), 1.0, 1e-6);
    EXPECT_NEAR(cues.front().end.seconds(), 2.5, 1e-6);

    const QString output = dir.filePath(QStringLiteral("output.srt"));
    ASSERT_TRUE(ccos::text::SubtitleParser::writeSrt(output, cues, &error)) << error.toStdString();
    EXPECT_TRUE(QFileInfo::exists(output));
}
