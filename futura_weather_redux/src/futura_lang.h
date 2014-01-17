#pragma once

/* The languages are: 
 * 0: "en_US"
 * 1: "sv_SE"
 * 2: "da_DK"
 * 3: "nl_NL"
 * 4: "fr_FR"
 * 5: "fi_FI"
 * 6: "de_DE"
 * 7: "it_IT"
 * 8: "es_ES"
 * 9: "no_NO"
 */

static const char *month_names[][12] = { 
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
    { "Jan", "Feb", "Mar", "Apr", "Maj", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec" },
    { "Jan", "Feb", "Mar", "Apr", "Maj", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec" },
    { "Jan", "Feb", "Mrt", "Apr", "Mei", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dec" },
    { "Janv", "FéVr", "Mars", "Avril", "Mai", "Juin", "Juil", "AoûT", "Sept", "Oct", "Nov", "DéC" },
    { "Tammi", "Helmi", "Maalis", "Huhti", "Touko", "Kesä", "Heinä", "Elo", "Syys", "Loka", "Marras", "Joulu" },
    { "Jan", "Feb", "MäR", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" },
    { "Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic" },
    { "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic" },
    { "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic" }
};



static const char *day_names[][7] = { 
    { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" },
    { "Mån", "Tis", "Ons", "Tor", "Fre", "Lör", "Sön" },
    { "Man", "Tir", "Ons", "Tor", "Fre", "Lør", "Søn" },
    { "Ma", "Di", "Wo", "Do", "Vr", "Za", "Zo" },
    { "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim" },
    { "Ma", "Ti", "Ke", "To", "Pe", "La", "Su" },
    { "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So" },
    { "Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom" },
    { "Lun", "Mar", "Mié", "Jue", "Vie", "SáB", "Dom" },
    { "Lun", "Mar", "Mié", "Jue", "Vie", "SáB", "Dom" }
};
