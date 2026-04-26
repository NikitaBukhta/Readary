# -*- coding: utf-8 -*-
"""Генерация PDF с требованиями к продукту на основе use-case диаграммы."""

from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib.enums import TA_JUSTIFY, TA_LEFT, TA_CENTER
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle, KeepTogether
)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
import os

# Регистрация шрифтов с поддержкой кириллицы
FONT_CANDIDATES = [
    ("DejaVuSans", r"C:\Windows\Fonts\DejaVuSans.ttf"),
    ("ArialUni", r"C:\Windows\Fonts\ARIALUNI.TTF"),
    ("Arial", r"C:\Windows\Fonts\arial.ttf"),
    ("Calibri", r"C:\Windows\Fonts\calibri.ttf"),
    ("Verdana", r"C:\Windows\Fonts\verdana.ttf"),
    ("Tahoma", r"C:\Windows\Fonts\tahoma.ttf"),
    ("SegoeUI", r"C:\Windows\Fonts\segoeui.ttf"),
]

FONT_BOLD_CANDIDATES = [
    ("Arial-Bold", r"C:\Windows\Fonts\arialbd.ttf"),
    ("Calibri-Bold", r"C:\Windows\Fonts\calibrib.ttf"),
    ("Verdana-Bold", r"C:\Windows\Fonts\verdanab.ttf"),
    ("Tahoma-Bold", r"C:\Windows\Fonts\tahomabd.ttf"),
    ("SegoeUI-Bold", r"C:\Windows\Fonts\segoeuib.ttf"),
]

FONT_NAME = None
FONT_BOLD = None

for name, path in FONT_CANDIDATES:
    if os.path.exists(path):
        try:
            pdfmetrics.registerFont(TTFont(name, path))
            FONT_NAME = name
            break
        except Exception:
            continue

for name, path in FONT_BOLD_CANDIDATES:
    if os.path.exists(path):
        try:
            pdfmetrics.registerFont(TTFont(name, path))
            FONT_BOLD = name
            break
        except Exception:
            continue

if FONT_NAME is None:
    FONT_NAME = "Helvetica"
if FONT_BOLD is None:
    FONT_BOLD = FONT_NAME

# Стили
styles = getSampleStyleSheet()

title_style = ParagraphStyle(
    'TitleStyle',
    parent=styles['Title'],
    fontName=FONT_BOLD,
    fontSize=22,
    leading=28,
    alignment=TA_CENTER,
    spaceAfter=16,
    textColor=colors.HexColor('#1F3A68'),
)

subtitle_style = ParagraphStyle(
    'SubtitleStyle',
    parent=styles['Normal'],
    fontName=FONT_NAME,
    fontSize=12,
    leading=16,
    alignment=TA_CENTER,
    spaceAfter=24,
    textColor=colors.HexColor('#555555'),
)

h1_style = ParagraphStyle(
    'H1',
    parent=styles['Heading1'],
    fontName=FONT_BOLD,
    fontSize=16,
    leading=20,
    spaceBefore=14,
    spaceAfter=8,
    textColor=colors.HexColor('#1F3A68'),
)

h2_style = ParagraphStyle(
    'H2',
    parent=styles['Heading2'],
    fontName=FONT_BOLD,
    fontSize=13,
    leading=16,
    spaceBefore=10,
    spaceAfter=6,
    textColor=colors.HexColor('#2D5490'),
)

normal_style = ParagraphStyle(
    'Normal_',
    parent=styles['Normal'],
    fontName=FONT_NAME,
    fontSize=10.5,
    leading=14,
    alignment=TA_JUSTIFY,
    spaceAfter=4,
)

bullet_style = ParagraphStyle(
    'Bullet',
    parent=normal_style,
    leftIndent=18,
    bulletIndent=6,
    spaceAfter=3,
)

sub_bullet_style = ParagraphStyle(
    'SubBullet',
    parent=normal_style,
    leftIndent=36,
    bulletIndent=24,
    spaceAfter=2,
    fontSize=10,
)


def p(text, style=normal_style):
    return Paragraph(text, style)


def bullet(text, level=0):
    style = bullet_style if level == 0 else sub_bullet_style
    return Paragraph(f"&bull;&nbsp;&nbsp;{text}", style)


def build_story():
    story = []

    # Титульная страница
    story.append(Spacer(1, 4 * cm))
    story.append(p("Требования к продукту", title_style))
    story.append(p("Мобильное приложение для учёта и чтения книг<br/>«DarieszzBooks»", subtitle_style))
    story.append(Spacer(1, 1 * cm))

    meta_data = [
        ["Версия документа:", "1.0"],
        ["Источник:", "use-case_diagram.drawio"],
        ["Тип документа:", "Функциональные требования"],
        ["Актор:", "Пользователь"],
    ]
    meta_table = Table(meta_data, colWidths=[5 * cm, 10 * cm])
    meta_table.setStyle(TableStyle([
        ('FONTNAME', (0, 0), (-1, -1), FONT_NAME),
        ('FONTSIZE', (0, 0), (-1, -1), 11),
        ('FONTNAME', (0, 0), (0, -1), FONT_BOLD),
        ('TEXTCOLOR', (0, 0), (0, -1), colors.HexColor('#1F3A68')),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 8),
        ('TOPPADDING', (0, 0), (-1, -1), 8),
        ('LINEBELOW', (0, 0), (-1, -1), 0.25, colors.HexColor('#DDDDDD')),
    ]))
    story.append(meta_table)
    story.append(PageBreak())

    # 1. Общее описание
    story.append(p("1. Общее описание продукта", h1_style))
    story.append(p(
        "«DarieszzBooks» — мобильное приложение для ведения личной библиотеки, "
        "учёта прочитанных и желаемых книг, постановки читательских целей, "
        "прохождения челленджей и комфортного процесса чтения. "
        "Приложение позволяет отслеживать прогресс, просматривать статистику "
        "и персонализировать интерфейс под предпочтения пользователя.",
        normal_style
    ))

    story.append(p("Основные акторы", h2_style))
    story.append(bullet("<b>Пользователь</b> — основной актор, взаимодействующий со всеми функциями приложения."))

    story.append(p("Ключевые функциональные блоки", h2_style))
    modules = [
        "Настройки приложения (темы, оформление)",
        "Поиск и добавление книг",
        "Категоризация книг («Хочу прочитать», «Хочу купить», прочитанные)",
        "Просмотр детальной информации о книге",
        "Процесс чтения (старт/стоп, режим концентрации)",
        "Читательские цели и челленджи",
        "Статистика чтения",
        "Виртуальные стеллажи и списки",
    ]
    for m in modules:
        story.append(bullet(m))

    story.append(PageBreak())

    # 2. Настройки приложения
    story.append(p("2. Настройки приложения", h1_style))

    story.append(p("FR-2.1. Выбор темы приложения", h2_style))
    story.append(p("Пользователь может выбирать разные темы оформления приложения.", normal_style))
    story.append(bullet("Система должна предоставлять несколько вариантов тем."))
    story.append(bullet("Смена темы применяется ко всему интерфейсу."))

    story.append(p("FR-2.2. Доступ к экрану настроек", h2_style))
    story.append(p("Пользователь может перейти в настройки приложения.", normal_style))

    story.append(p("FR-2.3. Настройка заднего фона для книги", h2_style))
    story.append(p(
        "Через настройки пользователь может выбрать оформление заднего фона "
        "для страниц книг.",
        normal_style
    ))
    story.append(bullet("<b>По цвету жанра</b> — фон подбирается автоматически в соответствии с жанром книги."))
    story.append(bullet("<b>Блюр</b> — размытый фон (например, на основе обложки книги)."))
    story.append(bullet("<b>Отключить</b> — отображать без заднего фона."))

    story.append(PageBreak())

    # 3. Поиск и добавление книг
    story.append(p("3. Поиск и добавление книг", h1_style))

    story.append(p("FR-3.1. Поиск книги", h2_style))
    story.append(p("Пользователь может искать книги в приложении.", normal_style))
    story.append(bullet("Поиск должен быть доступен с главного экрана."))
    story.append(bullet("Результаты поиска отображаются в виде списка книг."))

    story.append(p("FR-3.2. Добавление своей книги", h2_style))
    story.append(p(
        "Если книга не найдена в общей базе, пользователь может вручную "
        "добавить свою книгу с указанием метаданных.",
        normal_style
    ))

    story.append(p("FR-3.3. Добавление книги в категорию «Хочу прочитать»", h2_style))
    story.append(p(
        "Из результатов поиска или из карточки книги пользователь может "
        "добавить книгу в категорию «Хочу прочитать».",
        normal_style
    ))

    story.append(p("FR-3.4. Добавление книги в категорию «Хочу купить»", h2_style))
    story.append(p(
        "Из результатов поиска пользователь может добавить книгу в категорию "
        "«Хочу купить».",
        normal_style
    ))

    story.append(p("FR-3.5. Просмотр списка книг в результатах", h2_style))
    story.append(p("Для каждой книги в списке отображается:", normal_style))
    story.append(bullet("Название книги"))
    story.append(bullet("Обложка"))
    story.append(bullet("Количество страниц"))
    story.append(bullet("Автор"))
    story.append(bullet("Неполная аннотация (превью)"))

    story.append(PageBreak())

    # 4. Категории книг
    story.append(p("4. Категории и списки книг", h1_style))

    story.append(p("FR-4.1. Категория «Хочу прочитать»", h2_style))
    story.append(p(
        "Отдельная категория для книг, которые пользователь планирует "
        "прочитать в будущем.",
        normal_style
    ))
    story.append(bullet("Пользователь может открыть категорию и увидеть список книг."))
    story.append(bullet("Возможность добавить книгу в категорию из поиска или карточки книги."))
    story.append(bullet("Возможность перейти к просмотру информации о книге из списка."))

    story.append(p("FR-4.2. Категория «Хочу купить»", h2_style))
    story.append(p(
        "Отдельная категория для книг, которые пользователь планирует приобрести.",
        normal_style
    ))
    story.append(bullet("Пользователь может просматривать список книг категории."))
    story.append(bullet("Возможность добавления книги в категорию из поиска."))

    story.append(p("FR-4.3. Категория прочитанных книг", h2_style))
    story.append(p("Пользователь может просматривать список прочитанных книг.", normal_style))
    story.append(bullet("Просмотр прочитанных книг в виде списка."))
    story.append(bullet("Просмотр прочитанных книг в виде <b>виртуального стеллажа</b>."))
    story.append(bullet("Просмотр прочитанных книг в виде <b>виртуального списка</b>."))
    story.append(bullet("Переход к карточке книги из любого представления."))

    story.append(PageBreak())

    # 5. Карточка книги
    story.append(p("5. Просмотр информации о книге", h1_style))

    story.append(p("FR-5.1. Карточка книги — метаданные", h2_style))
    story.append(p("При открытии книги должны отображаться следующие метаданные:", normal_style))
    story.append(bullet("Обложка"))
    story.append(bullet("Автор"))
    story.append(bullet("Название"))
    story.append(bullet("Аннотация (полная)"))
    story.append(bullet("Год издания"))
    story.append(bullet("Издательство"))
    story.append(bullet("Рейтинг"))

    story.append(p("FR-5.2. Управление рейтингом", h2_style))
    story.append(bullet("Пользователь может <b>изменить личный рейтинг</b> книги."))
    story.append(bullet("Пользователь может <b>просмотреть глобальный рейтинг</b> книги (средний по всем пользователям)."))

    story.append(p("FR-5.3. PDF-версия книги", h2_style))
    story.append(p(
        "В карточке книги должна быть возможность перейти к PDF-версии книги "
        "(если она доступна).",
        normal_style
    ))

    story.append(p("FR-5.4. Персонажи книги", h2_style))
    story.append(p("Пользователь может работать со списком персонажей книги:", normal_style))
    story.append(bullet("Просмотреть список персонажей книги."))
    story.append(bullet("Добавить нового персонажа."))
    story.append(bullet("Удалить персонажа."))
    story.append(bullet("Изменить персонажа (редактирование атрибутов)."))

    story.append(p("FR-5.5. Статистика по книге", h2_style))
    story.append(p(
        "Из карточки книги должен быть доступен переход к статистике "
        "чтения для данной книги.",
        normal_style
    ))

    story.append(PageBreak())

    # 6. Процесс чтения
    story.append(p("6. Процесс чтения", h1_style))

    story.append(p("FR-6.1. Старт/Стоп чтения книги", h2_style))
    story.append(p(
        "Пользователь может запускать и останавливать сессию чтения книги. "
        "Время сессии учитывается в статистике.",
        normal_style
    ))

    story.append(p("FR-6.2. Режим концентрации", h2_style))
    story.append(p(
        "Режим, обеспечивающий комфортное сосредоточенное чтение. "
        "При его активации доступны следующие возможности:",
        normal_style
    ))

    story.append(p("FR-6.2.1. Фоновая музыка", h2_style))
    story.append(bullet("Включение фоновой музыки во время чтения."))
    story.append(bullet("Стандартный набор композиций: звук дождя, камина, природы и т.п.", level=1))
    story.append(bullet("Опционально — подключение собственного плейлиста пользователя.", level=1))

    story.append(p("FR-6.2.2. Режим «Не беспокоить»", h2_style))
    story.append(bullet("Отключение уведомлений (активация режима «Не беспокоить»)."))

    story.append(PageBreak())

    # 7. Цели и челленджи
    story.append(p("7. Читательские цели и челленджи", h1_style))

    story.append(p("FR-7.1. Добавление цели на определённый период", h2_style))
    story.append(p(
        "Пользователь может устанавливать читательскую цель на заданный "
        "временной период.",
        normal_style
    ))

    story.append(p("FR-7.1.1. Выбор периода", h2_style))
    story.append(bullet("Пользователь задаёт период цели (например, неделя, месяц, год или произвольный диапазон)."))

    story.append(p("FR-7.1.2. Тип цели", h2_style))
    story.append(bullet("Цель по <b>количеству книг</b>."))
    story.append(bullet("Цель по <b>количеству страниц</b>."))
    story.append(bullet("<b>Своя цель</b> — произвольный пользовательский тип."))

    story.append(p("FR-7.2. Челленджи", h2_style))
    story.append(p("Модуль челленджей для поддержания мотивации чтения.", normal_style))

    story.append(p("FR-7.2.1. Просмотр челленджей", h2_style))
    story.append(bullet("Просмотр всех челленджей."))
    story.append(bullet("Просмотр <b>встроенных</b> (предустановленных) челленджей."))
    story.append(bullet("Просмотр <b>кастомных</b> (созданных пользователем) челленджей."))

    story.append(p("FR-7.2.2. Автоматизация за выполнение челленджа", h2_style))
    story.append(p(
        "Пользователь может настроить автоматизацию (действие / награду), "
        "срабатывающую при выполнении челленджа.",
        normal_style
    ))

    story.append(PageBreak())

    # 8. Статистика
    story.append(p("8. Статистика чтения", h1_style))

    story.append(p("FR-8.1. Просмотр статистики", h2_style))
    story.append(p(
        "Пользователь может просматривать статистику своей читательской "
        "активности. Статистика доступна как из главного меню, так и из "
        "карточки конкретной книги.",
        normal_style
    ))

    story.append(p("FR-8.2. Показатели статистики", h2_style))
    story.append(bullet("<b>Количество прочитанных книг за период</b>."))
    story.append(bullet("<b>Скорость чтения за час:</b>"))
    story.append(bullet("Минимальная скорость.", level=1))
    story.append(bullet("Средняя скорость.", level=1))
    story.append(bullet("Максимальная скорость.", level=1))

    story.append(PageBreak())

    # 9. Сводная таблица use-cases
    story.append(p("9. Сводная матрица use-cases", h1_style))
    story.append(p(
        "Итоговый список всех пользовательских сценариев, выявленных "
        "из use-case диаграммы.",
        normal_style
    ))

    uc_data = [
        ["ID", "Use Case", "Модуль"],
        ["UC-01", "Выбор разных тем приложения", "Настройки"],
        ["UC-02", "Зайти в настройки", "Настройки"],
        ["UC-03", "Задний фон для книги (по цвету жанра / блюр / отключить)", "Настройки"],
        ["UC-04", "Поиск книги", "Поиск"],
        ["UC-05", "Добавить свою книгу", "Поиск"],
        ["UC-06", "Просмотреть список книг", "Поиск"],
        ["UC-07", "Добавить книгу в категорию «Хочу прочитать»", "Категории"],
        ["UC-08", "Добавить книгу в категорию «Хочу купить»", "Категории"],
        ["UC-09", "Категория «Хочу прочитать»", "Категории"],
        ["UC-10", "Категория «Хочу купить»", "Категории"],
        ["UC-11", "Просмотреть прочитанные книги", "Категории"],
        ["UC-12", "Просмотреть виртуальный стеллаж прочитанных книг", "Категории"],
        ["UC-13", "Просмотреть виртуальный список книг", "Категории"],
        ["UC-14", "Просмотреть информацию о книге (метаданные)", "Книга"],
        ["UC-15", "Изменить рейтинг", "Книга"],
        ["UC-16", "Просмотреть глобальный рейтинг", "Книга"],
        ["UC-17", "PDF версия книги", "Книга"],
        ["UC-18", "Просмотреть персонажей книги", "Книга"],
        ["UC-19", "Добавить / удалить / изменить персонажа", "Книга"],
        ["UC-20", "Старт / стоп чтения книги", "Чтение"],
        ["UC-21", "Режим концентрации", "Чтение"],
        ["UC-22", "Включить фоновую музыку", "Чтение"],
        ["UC-23", "Стандартный набор композиций (дождь, камин и т.д.)", "Чтение"],
        ["UC-24", "Свой плейлист (опционально)", "Чтение"],
        ["UC-25", "Выключить уведомления (Не беспокоить)", "Чтение"],
        ["UC-26", "Добавить цель на определённый период", "Цели"],
        ["UC-27", "Выбор периода цели", "Цели"],
        ["UC-28", "Тип цели: количество книг", "Цели"],
        ["UC-29", "Тип цели: количество страниц", "Цели"],
        ["UC-30", "Своя цель", "Цели"],
        ["UC-31", "Челленджи", "Челленджи"],
        ["UC-32", "Просмотреть челленджи", "Челленджи"],
        ["UC-33", "Встроенные челленджи", "Челленджи"],
        ["UC-34", "Кастомные челленджи", "Челленджи"],
        ["UC-35", "Все челленджи", "Челленджи"],
        ["UC-36", "Автоматизация за выполнение челленджа", "Челленджи"],
        ["UC-37", "Просмотреть статистику", "Статистика"],
        ["UC-38", "Прочитанных книг за период", "Статистика"],
        ["UC-39", "Скорость чтения за час (мин, средняя, макс)", "Статистика"],
    ]

    # Оборачиваем длинный текст в Paragraph
    wrap_style = ParagraphStyle(
        'Wrap',
        parent=normal_style,
        fontSize=9,
        leading=11,
        alignment=TA_LEFT,
    )
    wrap_header = ParagraphStyle(
        'WrapHead',
        parent=normal_style,
        fontName=FONT_BOLD,
        fontSize=10,
        leading=12,
        alignment=TA_LEFT,
        textColor=colors.whitesmoke,
    )

    wrapped = []
    for i, row in enumerate(uc_data):
        if i == 0:
            wrapped.append([Paragraph(c, wrap_header) for c in row])
        else:
            wrapped.append([Paragraph(c, wrap_style) for c in row])

    uc_table = Table(wrapped, colWidths=[1.6 * cm, 10.5 * cm, 3.2 * cm], repeatRows=1)
    uc_table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F3A68')),
        ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
        ('GRID', (0, 0), (-1, -1), 0.4, colors.HexColor('#B5B5B5')),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.whitesmoke, colors.HexColor('#EEF3FB')]),
        ('TOPPADDING', (0, 0), (-1, -1), 6),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 6),
        ('LEFTPADDING', (0, 0), (-1, -1), 6),
        ('RIGHTPADDING', (0, 0), (-1, -1), 6),
    ]))
    story.append(uc_table)

    story.append(PageBreak())

    # 10. Нефункциональные соображения
    story.append(p("10. Нефункциональные замечания", h1_style))
    story.append(p(
        "Данные замечания не выведены напрямую из use-case диаграммы, но "
        "являются стандартными требованиями к продукту такого класса "
        "и рекомендуются к уточнению на этапе детализации.",
        normal_style
    ))
    story.append(bullet("Поддержка офлайн-режима для чтения и просмотра добавленных книг."))
    story.append(bullet("Синхронизация данных между устройствами пользователя."))
    story.append(bullet("Локализация интерфейса (как минимум русский язык)."))
    story.append(bullet("Адаптивность UI под разные размеры экранов."))
    story.append(bullet("Обеспечение приватности пользовательских данных (рейтинги, цели, статистика)."))
    story.append(bullet("Производительность: плавная работа списков (стеллаж, виртуальный список) при большом количестве книг."))

    return story


def main():
    output_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "Product_Requirements.pdf"
    )

    doc = SimpleDocTemplate(
        output_path,
        pagesize=A4,
        leftMargin=2 * cm,
        rightMargin=2 * cm,
        topMargin=1.8 * cm,
        bottomMargin=1.8 * cm,
        title="Требования к продукту DarieszzBooks",
        author="DarieszzBooks Team",
    )

    def add_page_number(canvas, doc_):
        canvas.saveState()
        canvas.setFont(FONT_NAME, 9)
        canvas.setFillColor(colors.HexColor('#777777'))
        page_num = canvas.getPageNumber()
        canvas.drawRightString(A4[0] - 2 * cm, 1.2 * cm, f"Страница {page_num}")
        canvas.drawString(2 * cm, 1.2 * cm, "DarieszzBooks — Требования к продукту")
        canvas.restoreState()

    story = build_story()
    doc.build(story, onFirstPage=add_page_number, onLaterPages=add_page_number)
    print(f"PDF saved to: {output_path}")


if __name__ == "__main__":
    main()
