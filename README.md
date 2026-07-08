# Mechatroniczny-LEJ-BOX-5.0
Projekt przedstawia system automatycznego nalewania napojów oparty na mikrokontrolerze ESP32. Układ steruje zaworem, mierzy czas przepływu cieczy i zapisuje wyniki zawodników. Komunikuje się z aplikacją internetową poprzez REST API, umożliwiając automatyczne prowadzenie rozgrywek, aktualizację rankingu oraz prezentację wyników w czasie rzeczywistym

Opis projektu

Projekt przedstawia system automatycznego nalewania napajów przeznaczony do obsługi rozgrywek i zawodów. Centralnym elementem układu jest mikrokontroler ESP32, który odpowiada za komunikację sieciową, sterowanie elementami wykonawczymi oraz rejestrację wyników uczestników.

Po stronie sprzętowej system składa się z serwomechanizmu pełniącego funkcję zaworu otwierającego i zamykającego przepływ cieczy, przepływomierza wyposażonego w czujnik Halla, wyświetlacza LCD 20x4 z interfejsem I²C oraz zestawu przycisków służących do rozpoczęcia pomiaru, pomijania gracza oraz resetowania kolejki. Zasilanie realizowane jest za pomocą akumulatora 12 V oraz przetwornic DC-DC dostosowujących napięcie do wymagań poszczególnych modułów.

ESP32 łączy się z siecią Wi-Fi i komunikuje się z aplikacją internetową za pomocą interfejsu REST API. Lista graczy pobierana jest z serwera, a aktualny uczestnik oraz kolejny zawodnik wyświetlani są na ekranie LCD. Po naciśnięciu przycisku START serwomechanizm otwiera zawór i rozpoczyna oczekiwanie na pojawienie się przepływu. W momencie wykrycia pierwszych impulsów z przepływomierza rozpoczynany jest pomiar czasu. Gdy przepływ ustanie, system oblicza wynik zawodnika i zapisuje go w bazie danych.

Po zakończeniu rundy wynik wraz z identyfikatorem i nazwą gracza zostaje przesłany na serwer, gdzie jest zapisywany i prezentowany na stronie internetowej. System obsługuje również pomijanie zawodników, reset kolejki oraz ręczne sterowanie zaworem. Dzięki integracji z bazą danych i aplikacją webową możliwe jest automatyczne prowadzenie rozgrywki, wyświetlanie aktualnych wyników oraz tworzenie rankingu uczestników w czasie rzeczywistym.


Proces modelowania 3D

Proces projektowania elementów mechanicznych został zrealizowany w programie Siemens NX. Na etapie opracowywania konstrukcji przyjęto założenie maksymalnej modularności urządzenia, dzięki czemu poszczególne elementy mogą być łatwo demontowane, wymieniane lub modyfikowane bez konieczności przebudowy całego układu. Większość części została zaprojektowana z myślą o wykonaniu metodą druku 3D, co umożliwiło szybkie prototypowanie, testowanie kolejnych wersji oraz ograniczenie kosztów produkcji.

Projekt był rozwijany iteracyjnie. Powstało kilka wersji poszczególnych komponentów, które różniły się geometrią, sposobem montażu oraz rozwiązaniami uszczelniającymi. Każda kolejna wersja była weryfikowana podczas testów praktycznych, a uzyskane wyniki stanowiły podstawę do dalszych modyfikacji modelu.

Jednym z głównych wyzwań było zapewnienie odpowiedniej szczelności zaworu. Początkowe prototypy wykazywały nieszczelności wynikające z luzów montażowych oraz charakterystyki elementów wykonanych metodą druku 3D. Problem został rozwiązany poprzez zastosowanie dwóch pierścieni uszczelniających (O-ringów) na wale zaworu, a następnie dodanie trzeciego O-ringu wewnątrz korpusu zaworu, co znacząco poprawiło szczelność i niezawodność działania układu.

Podczas projektowania uwzględniono również ograniczenia technologii druku 3D, takie jak kierunek drukowania, grubość ścianek, tolerancje wymiarowe, dobór wypełnienia oraz łatwość montażu. Pozwoliło to uzyskać elementy o odpowiedniej wytrzymałości przy jednoczesnym zachowaniu krótkiego czasu wydruku i niewielkiego zużycia materiału.

Ostateczna wersja konstrukcji została opracowana na podstawie doświadczeń z przeprowadzonych testów oraz analizy wcześniejszych prototypów. Kompletna dokumentacja modeli 3D została udostępniona w repozytorium GitHub, co umożliwia odtworzenie projektu, jego dalszy rozwój oraz wykorzystanie przez innych użytkowników.
