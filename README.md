# CO2 Mapping in 3D

## 📋 Description

Projet étudiant de cartographie 3D en temps réel de la concentration en CO₂ dans un espace intérieur. Le système utilise un tag mobile équipé d'un capteur CO₂ et d'un module de positionnement UWB (Ultra-Wideband) pour mesurer et visualiser la distribution spatiale du CO₂.

## 🎯 Objectif

Créer une cartographie interactive 3D de la qualité de l'air intérieur en combinant :
- **Positionnement UWB** : Localisation précise du tag mobile dans l'espace
- **Mesure CO₂** : Acquisition en temps réel de la concentration en CO₂
- **Visualisation** : Dashboard Streamlit avec cartographie 3D interactive

## 🏗️ Architecture du Système

### Matériel

#### Tag Mobile
- **Module UWB** : DW1000 (positionnement)
- **Capteur CO₂** : SenseAir S8 (mesure NDIR)
- **Communication** : Bluetooth vers ordinateur
- **Microcontrôleur** : Arduino/ESP32

#### Ancres Fixes
- **Modules UWB** : DW1000 (références de position)
- **Nombre** : Minimum 4 ancres pour positionnement 3D
- **Configuration** : Positionnement fixe dans l'espace

### Flux de Données

```
Tag Mobile (DW1000 + SenseAir S8)
    ↓ UWB ranging
Ancres Fixes (DW1000)
    ↓ Distances mesurées
Tag Mobile
    ↓ Bluetooth (distances + CO₂)
Ordinateur
    ↓ Traitement Python
Dashboard Streamlit (Visualisation 3D)
```

## 📁 Structure du Projet

```
C02-mapping-3D/
├── C02map/
│   ├── anchor/              # Code pour les ancres UWB
│   │   └── anchor.ino       # Firmware Arduino pour ancres
│   ├── tag/                 # Code pour le tag mobile
│   │   ├── tag.ino          # Firmware Arduino pour tag
│   │   ├── link.cpp         # Gestion communication UWB
│   │   └── link.h           # Header pour communication
│   └── visualization/       # Module de visualisation Python
│       ├── dashboard.py     # Dashboard Streamlit
│       └── detection.py     # Module de détection et traitement
├── .gitignore
└── README.md
```

## 🚀 Installation

### Prérequis

- Python 3.8+
- Arduino IDE ou PlatformIO
- Bibliothèques Arduino pour DW1000
- Bibliothèques Python (voir ci-dessous)

### Installation Python

```bash
# Cloner le repository
git clone https://github.com/AmauryGachod/C02-mapping-3D.git
cd C02-mapping-3D

# Installer les dépendances Python
pip install -r requirements.txt
# ou
make install
```

### Dépendances Python Principales

- `streamlit` : Dashboard web interactif
- `numpy` : Calculs numériques
- `pandas` : Manipulation de données
- `plotly` : Visualisation 3D interactive
- `pyserial` : Communication Bluetooth/Serial
- `scipy` : Algorithmes de triangulation

## 🔧 Configuration

### 1. Programmation des Ancres

```bash
# Ouvrir C02map/anchor/anchor.ino dans Arduino IDE
# Configurer l'ID unique de chaque ancre
# Téléverser sur chaque module
```

### 2. Programmation du Tag

```bash
# Ouvrir C02map/tag/tag.ino dans Arduino IDE
# Configurer les paramètres Bluetooth et capteurs
# Téléverser sur le tag mobile
```

### 3. Calibration du Système

- Positionner les ancres aux coins de la zone à cartographier
- Noter les coordonnées exactes de chaque ancre
- Mettre à jour les positions dans le code Python

## 💻 Utilisation

### Lancement du Dashboard

```bash
# Lancer le dashboard Streamlit
streamlit run C02map/visualization/dashboard.py
```

Le dashboard sera accessible à l'adresse `http://localhost:8501`

### Acquisition de Données

1. **Démarrer les ancres** : Mettre sous tension les 4+ ancres fixes
2. **Démarrer le tag** : Allumer le tag mobile
3. **Connexion Bluetooth** : Connecter le tag à l'ordinateur via Bluetooth
4. **Démarrage acquisition** : Lancer le dashboard et commencer la collecte
5. **Déplacement** : Se déplacer dans la zone avec le tag mobile

### Visualisation

Le dashboard Streamlit affiche :
- **Carte 3D interactive** : Visualisation en temps réel de la position et du CO₂
- **Heatmap** : Cartographie de la concentration en CO₂
- **Graphiques temporels** : Evolution des mesures dans le temps
- **Statistiques** : Min, max, moyenne des concentrations

## 🧮 Algorithmes

### Triangulation UWB

Utilisation de la **multilatération 3D** basée sur :
- Mesures de distance (ToF - Time of Flight) entre tag et ancres
- Algorithme de moindres carrés pour optimisation
- Filtrage de Kalman pour réduction du bruit (optionnel)

### Interpolation Spatiale

Pour la cartographie continue du CO₂ :
- Interpolation par krigeage ou splines
- Agrégation temporelle des mesures
- Lissage spatial pour visualisation

## 📊 Spécifications Techniques

### Performances

- **Précision positionnement** : ±10-30 cm (selon configuration)
- **Fréquence d'échantillonnage CO₂** : 1 Hz (SenseAir S8)
- **Fréquence ranging UWB** : 10-100 Hz (configurable)
- **Portée UWB** : 50-200m (ligne de vue)

### Capteur SenseAir S8

- **Plage de mesure** : 0-10000 ppm
- **Précision** : ±40 ppm ±3% de la lecture
- **Technologie** : NDIR (Non-Dispersive Infrared)
- **Interface** : UART

### Module DW1000

- **Technologie** : IEEE 802.15.4-2011 UWB
- **Bandes de fréquence** : 3.5-6.5 GHz
- **Interface** : SPI

## 🛠️ Développement

### Branches Git

- `master` : Branche principale stable
- `develop` : Développement en cours

### Tests

```bash
# Lancer les tests unitaires
python -m pytest tests/
```

## 📝 TODO

### Priorité Haute
- [x] Protocole de communication UWB ↔ Tag
- [x] Algorithme de triangulation 3D
- [x] Lecture capteur CO₂ SenseAir S8
- [x] Dashboard Streamlit avec visualisation 3D
- [x] Communication Bluetooth Tag → PC

### Priorité Moyenne
- [ ] Gestion optimisée de la batterie
- [ ] Calibration automatique des ancres
- [ ] Export des données (CSV, JSON)
- [ ] Mode enregistrement/replay

### Priorité Basse
- [ ] Boîtiers 3D pour ancres et tag
- [ ] Interface web publique
- [ ] Support multi-tags
- [ ] Application mobile

## 🤝 Contributeurs

- **AmauryGachod** - Développement principal
- **JeanCHDJdev** - Code Arduino et intégration matérielle
- **emmaguetta** - Projet original

## 📄 Licence

Ce projet est développé dans le cadre d'un projet étudiant.

## 🔗 Ressources

### Documentation Matériel
- [DW1000 User Manual](https://www.decawave.com/)
- [SenseAir S8 Datasheet](https://senseair.com/)

### Bibliothèques Utilisées
- [DW1000 Arduino Library](https://github.com/thotro/arduino-dw1000)
- [Streamlit Documentation](https://docs.streamlit.io/)
- [Plotly Python Documentation](https://plotly.com/python/)

---

**Note** : Ce projet est un système expérimental développé à des fins éducatives. Pour une utilisation en conditions réelles, une validation et calibration approfondies sont nécessaires.
