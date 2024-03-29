# Generated by Django 2.0.7 on 2019-12-11 14:53

import datetime
from django.db import migrations, models
from django.utils.timezone import utc


class Migration(migrations.Migration):

    dependencies = [
        ('kalendarz', '0004_auto_20191211_1452'),
    ]

    operations = [
        migrations.AlterField(
            model_name='wydarzenie',
            name='data_wydarzenia',
            field=models.DateField(default=datetime.datetime(2019, 12, 11, 14, 53, 0, 275504, tzinfo=utc)),
        ),
        migrations.AlterField(
            model_name='wydarzenie',
            name='godz_rozpoczecia',
            field=models.TimeField(default=datetime.datetime(2019, 12, 11, 14, 53, 0, 275527, tzinfo=utc)),
        ),
        migrations.AlterField(
            model_name='wydarzenie',
            name='godz_zakonczenia',
            field=models.TimeField(default=datetime.datetime(2019, 12, 11, 14, 53, 0, 275543, tzinfo=utc)),
        ),
    ]
