# -*- coding: utf-8 -*-
"""Word-версия требований к продукту на основе use-case диаграммы."""

import os
from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_LINE_SPACING
from docx.enum.table import WD_ALIGN_VERTICAL
from docx.oxml.ns import qn
from docx.oxml import OxmlElement


PRIMARY = RGBColor(0x1F, 0x3A, 0x68)
SECONDARY = RGBColor(0x2D, 0x54, 0x90)
MUTED = RGBColor(0x55, 0x55, 0x55)
FONT = "Calibri"


def set_cell_shading(cell, color_hex):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), color_hex)
    tc_pr.append(shd)


def style_run(run, size=11, bold=False, color=None, font=FONT):
    run.font.name = font
    run.font.size = Pt(size)
    run.bold = bold
    if color is not None:
        run.font.color.rgb = color
    # Force eastAsia font for Cyrillic rendering consistency
    rpr = run._element.get_or_add_rPr()
    rfonts = rpr.find(qn('w:rFonts'))
    if rfonts is None:
        rfonts = OxmlElement('w:rFonts')
        rpr.append(rfonts)
    rfonts.set(qn('w:ascii'), font)
    rfonts.set(qn('w:hAnsi'), font)
    rfonts.set(qn('w:cs'), font)


def add_heading(doc, text, level=1):
    sizes = {1: 18, 2: 14, 3: 12}
    colors_map = {1: PRIMARY, 2: SECONDARY, 3: SECONDARY}
    p = doc.add_paragraph()
    p.paragraph_format.space_before = Pt(14 if level == 1 else 10)
    p.paragraph_format.space_after = Pt(6)
    p.paragraph_format.keep_with_next = True
    run = p.add_run(text)
    style_run(run, size=sizes.get(level, 12), bold=True, color=colors_map.get(level, PRIMARY))
    return p


def add_paragraph(doc, text, size=11, bold=False, align=None, space_after=4):
    p = doc.add_paragraph()
    if align is not None:
        p.alignment = align
    p.paragraph_format.space_after = Pt(space_after)
    p.paragraph_format.line_spacing_rule = WD_LINE_SPACING.MULTIPLE
    p.paragraph_format.line_spacing = 1.2
    runs = parse_inline(text)
    for text_part, is_bold in runs:
        run = p.add_run(text_part)
        style_run(run, size=size, bold=(bold or is_bold))
    return p


def parse_inline(text):
    """Парсинг простой <b>...</b> разметки в список (text, bold)."""
    parts = []
    cursor = 0
    while True:
        start = text.find("<b>", cursor)
        if start == -1:
            if cursor < len(text):
                parts.append((text[cursor:], False))
            break
        if start > cursor:
            parts.append((text[cursor:start], False))
        end = text.find("</b>", start)
        if end == -1:
            parts.append((text[start + 3:], True))
            break
        parts.append((text[start + 3:end], True))
        cursor = end + 4
    return parts


def add_bullet(doc, text, level=0):
    p = doc.add_paragraph()
    p.paragraph_format.left_indent = Cm(0.6 + level * 0.8)
    p.paragraph_format.space_after = Pt(2)
    p.paragraph_format.line_spacing_rule = WD_LINE_SPACING.MULTIPLE
    p.paragraph_format.line_spacing = 1.2
    bullet_char = "•" if level == 0 else "◦"
    prefix_run = p.add_run(f"{bullet_char}  ")
    style_run(prefix_run, size=11)
    for text_part, is_bold in parse_inline(text):
        run = p.add_run(text_part)
        style_run(run, size=11, bold=is_bold)
    return p


def add_title_page(doc):
    for _ in range(4):
        doc.add_paragraph()

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("Требования к продукту")
    style_run(run, size=28, bold=True, color=PRIMARY)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("Мобильное приложение для учёта и чтения книг")
    style_run(run, size=14, color=MUTED)

    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    run = p.add_run("«DarieszzBooks»")
    style_run(run, size=14, bold=True, color=MUTED)

    doc.add_paragraph()
    doc.add_paragraph()

    meta = [
        ("Версия документа:", "1.0"),
        ("Источник:", "use-case_diagram.drawio"),
        ("Тип документа:", "Функциональные требования"),
        ("Актор:", "Пользователь"),
    ]
    table = doc.add_table(rows=len(meta), cols=2)
    table.autofit = False
    table.columns[0].width = Cm(5.5)
    table.columns[1].width = Cm(10)
    for i, (k, v) in enumerate(meta):
        row = table.rows[i]
        row.cells[0].width = Cm(5.5)
        row.cells[1].width = Cm(10)
        c0 = row.cells[0].paragraphs[0]
        r0 = c0.add_run(k)
        style_run(r0, size=11, bold=True, color=PRIMARY)
        c1 = row.cells[1].paragraphs[0]
        r1 = c1.add_run(v)
        style_run(r1, size=11)

    doc.add_page_break()


def add_uc_table(doc, data):
    table = doc.add_table(rows=len(data), cols=3)
    table.style = 'Light Grid Accent 1'
    widths = [Cm(1.8), Cm(11.5), Cm(3.2)]
    for i, row in enumerate(data):
        tr = table.rows[i]
        for j, cell_text in enumerate(row):
            cell = tr.cells[j]
            cell.width = widths[j]
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER
            para = cell.paragraphs[0]
            para.paragraph_format.space_after = Pt(0)
            run = para.add_run(cell_text)
            if i == 0:
                style_run(run, size=10.5, bold=True, color=RGBColor(0xFF, 0xFF, 0xFF))
                set_cell_shading(cell, "1F3A68")
            else:
                style_run(run, size=10)
                if i % 2 == 0:
                    set_cell_shading(cell, "EEF3FB")


def add_footer(doc):
    section = doc.sections[0]
    footer = section.footer
    p = footer.paragraphs[0]
    p.alignment = WD_ALIGN_PARAGRAPH.LEFT
    run = p.add_run("DarieszzBooks — Требования к продукту")
    style_run(run, size=9, color=MUTED)
    # Добавим tab + номер страницы справа
    run2 = p.add_run("\t\t")
    style_run(run2, size=9)
    run3 = p.add_run()
    style_run(run3, size=9, color=MUTED)
    fldChar1 = OxmlElement('w:fldChar')
    fldChar1.set(qn('w:fldCharType'), 'begin')
    instrText = OxmlElement('w:instrText')
    instrText.text = 'PAGE'
    fldChar2 = OxmlElement('w:fldChar')
    fldChar2.set(qn('w:fldCharType'), 'end')
    run3._element.append(fldChar1)
    run3._element.append(instrText)
    run3._element.append(fldChar2)


def build_document():
    doc = Document()

    section = doc.sections[0]
    section.top_margin = Cm(2)
    section.bottom_margin = Cm(2)
    section.left_margin = Cm(2.2)
    section.right_margin = Cm(2.2)

    style = doc.styles['Normal']
    style.font.name = FONT
    style.font.size = Pt(11)

    add_footer(doc)
    add_title_page(doc)

    # 1. Общее описание
    add_heading(doc, "1. Общее описание продукта", 1)
    add_paragraph(doc,
        "«DarieszzBooks» — мобильное приложение для ведения личной библиотеки, "
        "учёта прочитанных и желаемых книг, постановки читательских целей, "
        "прохождения челленджей и комфортного процесса чтения. "
        "Приложение позволяет отслеживать прогресс, просматривать статистику "
        "и персонализировать интерфейс под предпочтения пользователя."
    )

    add_heading(doc, "Основные акторы", 2)
    add_bullet(doc, "<b>Пользователь</b> — основной актор, взаимодействующий со всеми функциями приложения.")

    add_heading(doc, "Ключевые функциональные блоки", 2)
    for m in [
        "Настройки приложения (темы, оформление)",
        "Поиск и добавление книг",
        "Категоризация книг («Хочу прочитать», «Хочу купить», прочитанные)",
        "Просмотр детальной информации о книге",
        "Процесс чтения (старт/стоп, режим концентрации)",
        "Читательские цели и челленджи",
        "Статистика чтения",
        "Виртуальные стеллажи и списки",
    ]:
        add_bullet(doc, m)

    doc.add_page_break()

    # 2. Настройки
    add_heading(doc, "2. Настройки приложения", 1)

    add_heading(doc, "FR-2.1. Выбор темы приложения", 2)
    add_paragraph(doc, "Пользователь может выбирать разные темы оформления приложения.")
    add_bullet(doc, "Система должна предоставлять несколько вариантов тем.")
    add_bullet(doc, "Смена темы применяется ко всему интерфейсу.")

    add_heading(doc, "FR-2.2. Доступ к экрану настроек", 2)
    add_paragraph(doc, "Пользователь может перейти в настройки приложения.")

    add_heading(doc, "FR-2.3. Настройка заднего фона для книги", 2)
    add_paragraph(doc,
        "Через настройки пользователь может выбрать оформление заднего фона "
        "для страниц книг."
    )
    add_bullet(doc, "<b>По цвету жанра</b> — фон подбирается автоматически в соответствии с жанром книги.")
    add_bullet(doc, "<b>Блюр</b> — размытый фон (например, на основе обложки книги).")
    add_bullet(doc, "<b>Отключить</b> — отображать без заднего фона.")

    doc.add_page_break()

    # 3. Поиск и добавление книг
    add_heading(doc, "3. Поиск и добавление книг", 1)

    add_heading(doc, "FR-3.1. Поиск книги", 2)
    add_paragraph(doc, "Пользователь может искать книги в приложении.")
    add_bullet(doc, "Поиск должен быть доступен с главного экрана.")
    add_bullet(doc, "Результаты поиска отображаются в виде списка книг.")

    add_heading(doc, "FR-3.2. Добавление своей книги", 2)
    add_paragraph(doc,
        "Если книга не найдена в общей базе, пользователь может вручную "
        "добавить свою книгу с указанием метаданных."
    )

    add_heading(doc, "FR-3.3. Добавление книги в категорию «Хочу прочитать»", 2)
    add_paragraph(doc,
        "Из результатов поиска или из карточки книги пользователь может "
        "добавить книгу в категорию «Хочу прочитать»."
    )

    add_heading(doc, "FR-3.4. Добавление книги в категорию «Хочу купить»", 2)
    add_paragraph(doc,
        "Из результатов поиска пользователь может добавить книгу в категорию "
        "«Хочу купить»."
    )

    add_heading(doc, "FR-3.5. Просмотр списка книг в результатах", 2)
    add_paragraph(doc, "Для каждой книги в списке отображается:")
    for item in ["Название книги", "Обложка", "Количество страниц", "Автор",
                 "Неполная аннотация (превью)"]:
        add_bullet(doc, item)

    doc.add_page_break()

    # 4. Категории
    add_heading(doc, "4. Категории и списки книг", 1)

    add_heading(doc, "FR-4.1. Категория «Хочу прочитать»", 2)
    add_paragraph(doc,
        "Отдельная категория для книг, которые пользователь планирует "
        "прочитать в будущем."
    )
    add_bullet(doc, "Пользователь может открыть категорию и увидеть список книг.")
    add_bullet(doc, "Возможность добавить книгу в категорию из поиска или карточки книги.")
    add_bullet(doc, "Возможность перейти к просмотру информации о книге из списка.")

    add_heading(doc, "FR-4.2. Категория «Хочу купить»", 2)
    add_paragraph(doc, "Отдельная категория для книг, которые пользователь планирует приобрести.")
    add_bullet(doc, "Пользователь может просматривать список книг категории.")
    add_bullet(doc, "Возможность добавления книги в категорию из поиска.")

    add_heading(doc, "FR-4.3. Категория прочитанных книг", 2)
    add_paragraph(doc, "Пользователь может просматривать список прочитанных книг.")
    add_bullet(doc, "Просмотр прочитанных книг в виде списка.")
    add_bullet(doc, "Просмотр прочитанных книг в виде <b>виртуального стеллажа</b>.")
    add_bullet(doc, "Просмотр прочитанных книг в виде <b>виртуального списка</b>.")
    add_bullet(doc, "Переход к карточке книги из любого представления.")

    doc.add_page_break()

    # 5. Карточка книги
    add_heading(doc, "5. Просмотр информации о книге", 1)

    add_heading(doc, "FR-5.1. Карточка книги — метаданные", 2)
    add_paragraph(doc, "При открытии книги должны отображаться следующие метаданные:")
    for item in ["Обложка", "Автор", "Название", "Аннотация (полная)",
                 "Год издания", "Издательство", "Рейтинг"]:
        add_bullet(doc, item)

    add_heading(doc, "FR-5.2. Управление рейтингом", 2)
    add_bullet(doc, "Пользователь может <b>изменить личный рейтинг</b> книги.")
    add_bullet(doc, "Пользователь может <b>просмотреть глобальный рейтинг</b> книги (средний по всем пользователям).")

    add_heading(doc, "FR-5.3. PDF-версия книги", 2)
    add_paragraph(doc,
        "В карточке книги должна быть возможность перейти к PDF-версии книги "
        "(если она доступна)."
    )

    add_heading(doc, "FR-5.4. Персонажи книги", 2)
    add_paragraph(doc, "Пользователь может работать со списком персонажей книги:")
    add_bullet(doc, "Просмотреть список персонажей книги.")
    add_bullet(doc, "Добавить нового персонажа.")
    add_bullet(doc, "Удалить персонажа.")
    add_bullet(doc, "Изменить персонажа (редактирование атрибутов).")

    add_heading(doc, "FR-5.5. Статистика по книге", 2)
    add_paragraph(doc,
        "Из карточки книги должен быть доступен переход к статистике "
        "чтения для данной книги."
    )

    doc.add_page_break()

    # 6. Процесс чтения
    add_heading(doc, "6. Процесс чтения", 1)

    add_heading(doc, "FR-6.1. Старт/Стоп чтения книги", 2)
    add_paragraph(doc,
        "Пользователь может запускать и останавливать сессию чтения книги. "
        "Время сессии учитывается в статистике."
    )

    add_heading(doc, "FR-6.2. Режим концентрации", 2)
    add_paragraph(doc,
        "Режим, обеспечивающий комфортное сосредоточенное чтение. "
        "При его активации доступны следующие возможности:"
    )

    add_heading(doc, "FR-6.2.1. Фоновая музыка", 3)
    add_bullet(doc, "Включение фоновой музыки во время чтения.")
    add_bullet(doc, "Стандартный набор композиций: звук дождя, камина, природы и т.п.", level=1)
    add_bullet(doc, "Опционально — подключение собственного плейлиста пользователя.", level=1)

    add_heading(doc, "FR-6.2.2. Режим «Не беспокоить»", 3)
    add_bullet(doc, "Отключение уведомлений (активация режима «Не беспокоить»).")

    doc.add_page_break()

    # 7. Цели и челленджи
    add_heading(doc, "7. Читательские цели и челленджи", 1)

    add_heading(doc, "FR-7.1. Добавление цели на определённый период", 2)
    add_paragraph(doc,
        "Пользователь может устанавливать читательскую цель на заданный "
        "временной период."
    )

    add_heading(doc, "FR-7.1.1. Выбор периода", 3)
    add_bullet(doc, "Пользователь задаёт период цели (например, неделя, месяц, год или произвольный диапазон).")

    add_heading(doc, "FR-7.1.2. Тип цели", 3)
    add_bullet(doc, "Цель по <b>количеству книг</b>.")
    add_bullet(doc, "Цель по <b>количеству страниц</b>.")
    add_bullet(doc, "<b>Своя цель</b> — произвольный пользовательский тип.")

    add_heading(doc, "FR-7.2. Челленджи", 2)
    add_paragraph(doc, "Модуль челленджей для поддержания мотивации чтения.")

    add_heading(doc, "FR-7.2.1. Просмотр челленджей", 3)
    add_bullet(doc, "Просмотр всех челленджей.")
    add_bullet(doc, "Просмотр <b>встроенных</b> (предустановленных) челленджей.")
    add_bullet(doc, "Просмотр <b>кастомных</b> (созданных пользователем) челленджей.")

    add_heading(doc, "FR-7.2.2. Автоматизация за выполнение челленджа", 3)
    add_paragraph(doc,
        "Пользователь может настроить автоматизацию (действие / награду), "
        "срабатывающую при выполнении челленджа."
    )

    doc.add_page_break()

    # 8. Статистика
    add_heading(doc, "8. Статистика чтения", 1)

    add_heading(doc, "FR-8.1. Просмотр статистики", 2)
    add_paragraph(doc,
        "Пользователь может просматривать статистику своей читательской "
        "активности. Статистика доступна как из главного меню, так и из "
        "карточки конкретной книги."
    )

    add_heading(doc, "FR-8.2. Показатели статистики", 2)
    add_bullet(doc, "<b>Количество прочитанных книг за период</b>.")
    add_bullet(doc, "<b>Скорость чтения за час:</b>")
    add_bullet(doc, "Минимальная скорость.", level=1)
    add_bullet(doc, "Средняя скорость.", level=1)
    add_bullet(doc, "Максимальная скорость.", level=1)

    doc.add_page_break()

    # 9. Сводная таблица use-cases
    add_heading(doc, "9. Сводная матрица use-cases", 1)
    add_paragraph(doc,
        "Итоговый список всех пользовательских сценариев, выявленных "
        "из use-case диаграммы."
    )

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
    add_uc_table(doc, uc_data)

    doc.add_page_break()

    # 10. Нефункциональные
    add_heading(doc, "10. Нефункциональные замечания", 1)
    add_paragraph(doc,
        "Данные замечания не выведены напрямую из use-case диаграммы, но "
        "являются стандартными требованиями к продукту такого класса "
        "и рекомендуются к уточнению на этапе детализации."
    )
    add_bullet(doc, "Поддержка офлайн-режима для чтения и просмотра добавленных книг.")
    add_bullet(doc, "Синхронизация данных между устройствами пользователя.")
    add_bullet(doc, "Локализация интерфейса (как минимум русский язык).")
    add_bullet(doc, "Адаптивность UI под разные размеры экранов.")
    add_bullet(doc, "Обеспечение приватности пользовательских данных (рейтинги, цели, статистика).")
    add_bullet(doc, "Производительность: плавная работа списков (стеллаж, виртуальный список) при большом количестве книг.")

    return doc


def main():
    output_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "Product_Requirements.docx"
    )
    doc = build_document()
    doc.save(output_path)
    print(f"DOCX saved to: {output_path}")


if __name__ == "__main__":
    main()
