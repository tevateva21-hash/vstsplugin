#include "PresetBar.h"
#include "PsyUI.h"

PresetBar::PresetBar (juce::AudioProcessorValueTreeState& a, const std::vector<FactoryPreset>& p, const juce::String& f, juce::StringArray ex)
    : apvts (a), presets (p), folder (f), excluded (ex)
{
    menuBtn.setButtonText ("Presets...");
    menuBtn.onClick = [this] { showMenu(); };
    prev.onClick = [this] { step (-1); }; next.onClick = [this] { step (1); };
    save.onClick = [this] { saveUser(); }; diceBtn.onClick = [this] { dice(); };
    for (auto* b : { &menuBtn, &prev, &next, &save, &diceBtn }) addAndMakeVisible (b);
    diceBtn.setTooltip ("Mutation aleatoire du preset courant");
}
void PresetBar::resized()
{
    auto r = getLocalBounds();
    prev.setBounds (r.removeFromLeft (28)); r.removeFromLeft (3);
    menuBtn.setBounds (r.removeFromLeft (230)); r.removeFromLeft (3);
    next.setBounds (r.removeFromLeft (28)); r.removeFromLeft (8);
    save.setBounds (r.removeFromLeft (56)); r.removeFromLeft (4);
    diceBtn.setBounds (r.removeFromLeft (56));
}
void PresetBar::paint (juce::Graphics&) {}

juce::File PresetBar::userDir() const
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("PsyLab").getChildFile (folder);
    d.createDirectory(); return d;
}
juce::Array<juce::File> PresetBar::userFiles() const { return userDir().findChildFiles (juce::File::findFiles, false, "*.psypreset"); }

void PresetBar::resetAll()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! excluded.contains (rp->paramID)) rp->setValueNotifyingHost (rp->getDefaultValue());
}
void PresetBar::applyFactory (int index)
{
    if (index < 0 || index >= (int) presets.size()) return;
    resetAll();
    for (auto& kv : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first)) p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    current = index;
    menuBtn.setButtonText (presets[(size_t) index].category + " / " + presets[(size_t) index].name);
}
void PresetBar::step (int dir)
{
    if (presets.empty()) return;
    applyFactory ((current < 0 ? (dir > 0 ? 0 : (int) presets.size() - 1) : current + dir + (int) presets.size()) % (int) presets.size());
}
void PresetBar::showMenu()
{
    juce::PopupMenu m; juce::StringArray cats;
    for (auto& p : presets) cats.addIfNotAlreadyThere (p.category);
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (size_t i = 0; i < presets.size(); ++i) if (presets[i].category == c) sub.addItem (1000 + (int) i, presets[i].name, true, (int) i == current);
        m.addSubMenu (c, sub);
    }
    juce::PopupMenu user; auto files = userFiles();
    for (int i = 0; i < files.size(); ++i) user.addItem (5000 + i, files[i].getFileNameWithoutExtension());
    if (files.isEmpty()) user.addItem (-1, "(aucun preset utilisateur)", false);
    m.addSubMenu ("Mes presets", user);
    m.addSeparator(); m.addItem (9000, "Sauvegarder le preset actuel...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuBtn), [this] (int r)
    {
        if (r >= 1000 && r < 5000) applyFactory (r - 1000);
        else if (r >= 5000 && r < 9000) { auto f = userFiles(); if (r - 5000 < f.size()) loadUser (f[r - 5000]); }
        else if (r == 9000) saveUser();
    });
}
void PresetBar::loadUser (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            menuBtn.setButtonText ("Mes presets / " + f.getFileNameWithoutExtension()); current = -1;
        }
}
void PresetBar::saveUser()
{
    auto* aw = new juce::AlertWindow ("Sauvegarder le preset", "Nom du preset :", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", "Mon preset"); aw->addButton ("Sauvegarder", 1); aw->addButton ("Annuler", 0);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int r)
    {
        if (r == 1)
        {
            auto n = juce::File::createLegalFileName (aw->getTextEditorContents ("name"));
            if (n.isNotEmpty())
                if (auto xml = apvts.copyState().createXml()) { xml->writeTo (userDir().getChildFile (n + ".psypreset")); menuBtn.setButtonText ("Mes presets / " + n); }
        }
    }), true);
}
void PresetBar::dice()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            if (excluded.contains (rp->paramID)) continue;
            if (dynamic_cast<juce::AudioParameterFloat*> (rp))
                rp->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, rp->getValue() + (rng.nextFloat() - 0.5f) * 0.5f));
            else if (rng.nextFloat() < 0.08f)
                rp->setValueNotifyingHost (rng.nextFloat());
        }
}
