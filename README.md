<div class="container">

## Zadanie 6

Celem zadania jest stworzenie urządzenia znakowego `/dev/adler`, które będzie liczyło sumę kontrolną [Adler32](https://pl.wikipedia.org/wiki/Adler-32).

W algorytmie Adler-32 trzymamy dwie zmienne: A i B, początkowo inicjowane odpowiednio na 1 i 0\. Taki stan zmiennych nazywamy stanem początkowym.

Operacja odczytu z urządzenia daje wartość sumy kontrolnej w formacie szesnastkowym (cyfry powyżej 9 pisne małymi literami) wszystkich danych, które się do urządzenia zapisało od ostatniego odczytu (lub uruchomienia urządzenia, jeśli nie było jeszcze żadnego odczytu). Stan urządzenia jest resetowany do stanu początkowego po każdym odczycie. Suma kontrolna ma 8 znaków i poprawna operacja odczytu daje w wyniku zawsze 8 bajtów. Operacja odczytu nie powinna się udać (nic nie powinno być przeczytane), jeśli rozmiar czytanych danych jest mniejszy od 8 znaków, i powinna zakończyć się wtedy błędem EINVAL – invalid argument.

Operacja zapisu do urządzenia pozwala na wpisanie danych, których sumę kontrolną chcemy policzyć. **Dane potencjalnie mogą być bardzo duże.**

Urządzenie powinno zachowywać aktualny stan w przypadku uruchomienia `service update`.

W `/etc/system.conf` zostanie umieszczony poniższy wpis:

<pre>service adler
{
        system
                IRQCTL          # 19
                DEVIO           # 21
        ;
        ipc
                SYSTEM pm rs tty ds vm vfs
                pci inet lwip amddev
                ;
    uid 0;
};
</pre>

Sterownik będzie kompilowany za pomocą dostarczonego przez nas Makefile, analogicznego jak dla przykładowego sterownika hello. Rozwiązanie powinno składać się z jednego pliku `adler.c`, który będzie zawierał implementację urządzenia znakowego. Plik razem z Makefile zostanie umieszczony w katalogu: `/usr/src/minix/drivers/adler`, gdzie zostaną wykonane polecenia:

<pre>make clean
make
make install

service up /service/adler
service update /service/adler
service down adler
</pre>

### Oddawanie zadania

Termin oddania: 10 czerwca 2019, godz. 20.

Rozwiązanie należy umieścić w pliku `adler.c` w repozytorium SVN w katalogu

<pre>studenci/ab123456/zadanie6
</pre>

gdzie ab123456 jest identyfikatorem studenta używanym do logowania w laboratorium komputerowym. Katalog ten nie powinien zawierać innych plików.

Z uwagi na automatyzację sprawdzania, rozwiązania niespełniające wymogów formalnych (termin oddania, nazwa pliku, nazwa katalogu w repozytorium itp.) otrzymają 0 punktów.

### Przykład

<pre># mknod /dev/adler c 20 0
# service up /service/adler -dev /dev/adler
# head -c 8 /dev/adler | xargs echo
00000001
# echo "Hello" > /dev/adler
# head -c 8 /dev/adler | xargs echo
078b01ff
# head -c 8 /dev/adler | xargs echo
00000001
</pre>

</div>
