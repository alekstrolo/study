# Generated by Django 3.0.6 on 2020-05-30 15:18

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('Dostawa_Infrastructure', '0002_auto_20200530_1516'),
    ]

    operations = [
        migrations.AlterField(
            model_name='packagemodel',
            name='_PackageCodeField',
            field=models.CharField(max_length=200),
        ),
    ]