# `.ui` XML Reference (QLC+ LEGACY)

Element order and attributes as actually used in this repo. Qt Designer writes this format; hand-editing is fine as long as the structure stays valid.

## Top-level order

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <author>Name</author>          <!-- optional but common -->
 <comment>... license ...</comment>
 <class>ClassName</class>       <!-- required; drives Ui_ClassName -->
 <widget class="QDialog" name="ClassName"> ... </widget>
 <resources> ... </resources>   <!-- optional -->
 <connections> ... </connections>
 <tabstops> ... </tabstops>     <!-- optional -->
 <customwidgets> ... </customwidgets> <!-- optional -->
</ui>
```

- `<class>` is the generated `Ui_<class>` type name. It normally equals the root widget `name` and the C++ class — but see the mismatch gotchas in `SKILL.md`.
- `<comment>` carries the Apache-2.0 header. Escape `"` as `&quot;`.

## Root widget

```xml
<widget class="QDialog" name="MyDialog">
 <property name="geometry">
  <rect><x>0</x><y>0</y><width>700</width><height>400</height></rect>
 </property>
 <property name="windowTitle">
  <string>Configure ArtNet Plugin</string>
 </property>
 <layout class="QVBoxLayout" name="verticalLayout_3"> ... </layout>
</widget>
```

- `QDialog` for dialogs, `QWidget` for embedded/reusable widgets.
- `geometry` is always present.
- Embedded widgets often use `<string notr="true">Form</string>` for `windowTitle`.
- A widget with a single child and no layout is valid, but prefer an explicit layout.

## Layouts

Only these three are used in the repo:

| Layout | Typical use |
|---|---|
| `QVBoxLayout` | vertical stacking (most dialogs) |
| `QHBoxLayout` | button rows, label+field pairs |
| `QGridLayout` | forms, tables of controls, `InputSelectionWidget` |

```xml
<layout class="QGridLayout" name="m_mainGrid">
 <property name="leftMargin"><number>0</number></property>
 <property name="topMargin"><number>0</number></property>
 <property name="rightMargin"><number>0</number></property>
 <property name="bottomMargin"><number>0</number></property>
 <item row="0" column="1">
  <widget class="QGroupBox" name="m_keyInputGroup"> ... </widget>
 </item>
</layout>
```

- `QVBoxLayout`/`QHBoxLayout` items use `<item>` without `row`/`column`.
- `QGridLayout` items use `<item row="R" column="C">`; add `rowspan`/`colspan` attributes when needed.
- Spacers: `<spacer name="..."><property name="orientation"><enum>Qt::Horizontal</enum></property><property name="sizeType"><enum>QSizePolicy::Expanding</enum></property><property name="sizeHint" stdset="0"><size><width>40</width><height>20</height></size></property></spacer>`.

## Common properties

```xml
<property name="sizePolicy">
 <sizepolicy hsizetype="Minimum" vsizetype="Preferred">
  <horstretch>0</horstretch>
  <verstretch>0</verstretch>
 </sizepolicy>
</property>
<property name="minimumSize"><size><width>0</width><height>0</height></size></property>
<property name="maximumSize"><size><width>16777215</width><height>16777215</height></size></property>
<property name="toolTip"><string>...</string></property>
<property name="text"><string>...</string></property>
<property name="checked"><bool>false</bool></property>
<property name="enabled"><bool>true</bool></property>
<property name="visible"><bool>true</bool></property>
```

- Numbers: `<number>`. Booleans: `<bool>`. Strings: `<string>`. Enums: `<enum>`.
- Non-translatable strings: `<string notr="true">`.

## Containers

### Tab widget

```xml
<widget class="QTabWidget" name="m_tabWidget">
 <property name="currentIndex"><number>0</number></property>
 <widget class="QWidget" name="m_tab">
  <attribute name="title"><string>Universes Configuration</string></attribute>
  <layout class="QVBoxLayout" name="m_tabLayout"> ... </layout>
 </widget>
</widget>
```

### Group box

```xml
<widget class="QGroupBox" name="m_groupBox">
 <property name="title"><string>Key combination</string></property>
 <layout class="QVBoxLayout" name="m_groupLayout"> ... </layout>
</widget>
```

### Tree / table columns

```xml
<widget class="QTreeWidget" name="m_uniMapTree">
 <column><property name="text"><string>Interface</string></property></column>
 <column><property name="text"><string>Universe</string></property></column>
</widget>
```

## Dialog button box

```xml
<widget class="QDialogButtonBox" name="m_buttonBox">
 <property name="orientation"><enum>Qt::Horizontal</enum></property>
 <property name="standardButtons">
  <set>QDialogButtonBox::Cancel|QDialogButtonBox::Ok</set>
 </property>
</widget>
```

Wire it in `<connections>`:

```xml
<connections>
 <connection>
  <sender>m_buttonBox</sender><signal>accepted()</signal>
  <receiver>MyDialog</receiver><slot>accept()</slot>
 </connection>
 <connection>
  <sender>m_buttonBox</sender><signal>rejected()</signal>
  <receiver>MyDialog</receiver><slot>reject()</slot>
 </connection>
</connections>
```

## Resources

Icons come from `ui/src/qlcui.qrc`. The `<include location>` is **relative to the `.ui` file's directory**:

| `.ui` location | `<include location=...>` |
|---|---|
| `ui/src/` | `qlcui.qrc` |
| `ui/src/monitor/` | `../qlcui.qrc` |
| `fixtureeditor/` | `../ui/src/qlcui.qrc` |
| `plugins/osc/` | `../../ui/src/qlcui.qrc` |

```xml
<resources>
 <include location="qlcui.qrc"/>
</resources>
```

Reference an icon with `<iconset resource="qlcui.qrc"><normaloff>:/icons/png/foo.png</normaloff>:/icons/png/foo.png</iconset>`.

## Custom widgets (promotion)

Only `SpeedDial` is promoted in this repo (`ui/src/virtualconsole/vcspeeddialproperties.ui`):

```xml
<customwidgets>
 <customwidget>
  <class>SpeedDial</class>
  <extends>QWidget</extends>
  <header>speeddial.h</header>
 </customwidget>
</customwidgets>
```

- `<extends>` must be the real base class.
- `<header>` is the include the generated code emits — use the bare header name as the C++ code includes it.
- The promoted widget must be a real QLC+ class with `Q_OBJECT`.

## Tab order

```xml
<tabstops>
 <tabstop>m_nameEdit</tabstop>
 <tabstop>m_sizeXSpin</tabstop>
 <tabstop>m_buttonBox</tabstop>
</tabstops>
```

Optional; present in ~10 files. Add it when a form has a non-obvious focus order.

## Enum forms

Both are accepted by uic:

- Unscoped: `<enum>Qt::Horizontal</enum>`, `<enum>QSizePolicy::Expanding</enum>`
- Scoped: `<enum>Qt::TextElideMode::ElideNone</enum>`, `<enum>QTabWidget::TabPosition::East</enum>`

Match the neighbouring file; do not mix within one file.
